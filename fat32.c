#include "types.h"
#include "string.h"
#include "bug.h"
#include "fat32.h"
#include "sdcard.h"

#include "jtag.h"

/* FAT EOC mark (End Of Clusterchain) */
#define EOC 0x0FFFFFF8

/* BPB Structure (FAT32) */
struct BPB32 {
	uint8_t		BS_jmpBoot[3];
	uint8_t		BS_OEMName[8];
	uint8_t		BPB_BytsPerSec[2];
	uint8_t		BPB_SecPerClus;
	uint8_t		BPB_RsvdSecCnt[2];
	uint8_t		BPB_NumFATs;
	uint8_t		BPB_RootEntCnt[2];
	uint8_t		BPB_TotSec16[2];
	uint8_t		BPB_Media;
	uint16_t	BPB_FATSz16;
	uint16_t	BPB_SecPerTrk;
	uint16_t	BPB_NumHeads;
	uint32_t	BPB_HiddSec;
	uint32_t	BPB_TotSec32;
	uint32_t	BPB_FATSz32;
	uint16_t	BPB_ExtFlags;
	uint8_t		BPB_FSVer[2];
	uint32_t	BPB_RootClus;
	uint8_t		rest[464];
};

/* FAT 32 Byte Directory Entry Structure */

#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_LONG_NAME	(ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_LONG_NAME | ATTR_DIRECTORY | ATTR_ARCHIVE)

#define LAST_LONG_ENTRY		0x40

struct DIRRECORD32 {
	uint8_t		DIR_Name[11];
	uint8_t		DIR_Attr;
	uint8_t		DIR_NTRes;
	uint8_t		DIR_CrtTimeTenth;
	uint16_t	DIR_CrtTime;
	uint16_t	DIR_CrtDate;
	uint16_t	DIR_LstAccDate;
	uint16_t	DIR_FstClusHI;
	uint16_t	DIR_WrtTime;
	uint16_t	DIR_WrtDate;
	uint16_t	DIR_FstClusLO;
	uint32_t	DIR_FileSize;
};

struct LONGDIRRECORD {
	uint8_t		LDIR_Ord;
	uint8_t		LDIR_Name1[10];
	uint8_t		LDIR_Attr;
	uint8_t		LDIR_Type;
	uint8_t		LDIR_Chksum;
	uint8_t		LDIR_Name2[12];
	uint8_t		LDIR_FstClusLO[2];
	uint8_t		LDIR_Name3[4];
};

typedef enum {DIRECTORY, FILE} open_t;


static struct {
	uint8_t sec_per_clus;
	uint32_t first_cluster_lba;
	uint32_t rootdir_clusnum;
	uint32_t fat_lba;

	uint8_t fatbuf[SECTOR_SIZE];
	uint32_t fatbuf_lba;

	uint32_t buf[SECTOR_SIZE/4];
	uint32_t buf_lba;
} FAT32_S;


static uint32_t clusnum_to_lba(uint32_t cluster_num)
{
	if (cluster_num < 2 || cluster_num >= EOC)
		bug();

	return FAT32_S.first_cluster_lba
		+ (cluster_num - 2) * FAT32_S.sec_per_clus;
}

static uint32_t next_cluster(uint32_t cluster_num)
{
	uint32_t offset, lba, next;

	if (cluster_num < 2 || cluster_num >= EOC)
		bug();

	offset = cluster_num / (SECTOR_SIZE/4);
	lba = FAT32_S.fat_lba + offset;

	if (FAT32_S.fatbuf_lba != lba) {
		if (SD_read_lba((void *) &FAT32_S.fatbuf, lba)) {
			put_string("next_cluster: SD_read_lba failed!!\n");
			bug();
		}
		FAT32_S.fatbuf_lba = lba;
	}

	memcpy((void *) &next,
		(void *)(FAT32_S.fatbuf + (cluster_num - offset
						* SECTOR_SIZE/4) * 4),
		sizeof(next));

	return next & 0x0FFFFFFF;
}

static void buffer_file_sector(struct FAT32_FILE *f)
{
	uint32_t lba = clusnum_to_lba(f->cur_cluster) + f->cur_sector;

	if (FAT32_S.buf_lba != lba) {
		FAT32_S.buf_lba = lba;
		if (SD_read_lba((void *)FAT32_S.buf, lba)) {
			put_string("buffer_file_sector: SD_read_lba failed\n");
			bug();
		}
	}
}

int fat32_init()
{
	struct BPB32 *bpb = (struct BPB32*)FAT32_S.buf;

	if (SD_read_lba((void *) bpb, 0))
		return -1;

	if (((uint16_t)bpb->BPB_BytsPerSec[1] << 8) + bpb->BPB_BytsPerSec[0] 
			!= SECTOR_SIZE)
		return -1;

	FAT32_S.sec_per_clus = bpb->BPB_SecPerClus;
	FAT32_S.rootdir_clusnum = bpb->BPB_RootClus;
	FAT32_S.fat_lba = ((uint32_t)bpb->BPB_RsvdSecCnt[1] << 8) 
				+ bpb->BPB_RsvdSecCnt[0];
	FAT32_S.first_cluster_lba = FAT32_S.fat_lba + bpb->BPB_NumFATs
					* bpb->BPB_FATSz32;
	FAT32_S.fatbuf_lba = 0;
	FAT32_S.buf_lba = 0;

	return 0;
}

static void conv83(const char *name83, char *dest)
{
	char buf[16];
	int i, j;

	for (i = 0; i < 11; i++) {
		if (name83[i] == ' ') break;
		buf[i] = name83[i];
	}

	if (name83[8] != ' ') {
		buf[i++] = '.';
		for (j = 8; j < 11; j++, i++) {
			if (name83[j] == ' ') break;
			buf[i] = name83[j];
		}
	}

	buf[i] = '\0';

	strcpy(dest, buf);
}

static int give_dirrecord(struct FAT32_DIR *dir, struct DIRRECORD32 *d)
{
	struct DIRRECORD32 *dr = (struct DIRRECORD32 *)FAT32_S.buf;
	uint32_t cluster = dir->cur_cluster;
	uint32_t sector = dir->cur_sector;
	uint32_t lba;
	unsigned i = dir->position;

	if (cluster >= EOC)
		return -1;

	lba = clusnum_to_lba(cluster) + sector;

	if (FAT32_S.buf_lba != lba) {
		FAT32_S.buf_lba = lba;
		if (SD_read_lba((void *)FAT32_S.buf, lba))
			return -1;
	}

	memcpy(d, dr+i, sizeof(struct DIRRECORD32));

	i++;
	if (i == SECTOR_SIZE/sizeof(struct DIRRECORD32)) {
		i = 0;
		sector++;
		if (sector == FAT32_S.sec_per_clus) {
			sector = 0;
			cluster = next_cluster(cluster);
		}
	}

	dir->cur_cluster = cluster;
	dir->cur_sector = sector;
	dir->position = i;

	return 0;
}

static uint8_t gen_chksum(const char *name83)
{
	int i;
	uint8_t sum = 0;

	for (i = 11; i != 0; i--) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *name83++;
	}

	return sum;
}

// TODO: korjaa tämä karseus
static int extract_lfname(const struct LONGDIRRECORD *ldr, char *buf, int i)
{
	int j, a = 0;
	
	// LDIR_Name3
	for (j = 3; j >= 0; j -= 2) {
		if ((ldr->LDIR_Name3[j] == 0xFF
				&& ldr->LDIR_Name3[j-1] == 0xFF)
				|| (ldr->LDIR_Name3[j] == 0x00
				&& ldr->LDIR_Name3[j-1] == 0x00))
			continue;
		if (ldr->LDIR_Name3[j] != 0x00) {
			return -1;
		}
		buf[i] = ldr->LDIR_Name3[j-1];
		if (--i < 0) return -1;
		a++;
	}
	// LDIR_Name2
	for (j = 11; j >= 0; j -= 2) {
		if ((ldr->LDIR_Name2[j] == 0xFF
				&& ldr->LDIR_Name2[j-1] == 0xFF)
				|| (ldr->LDIR_Name2[j] == 0x00
				&& ldr->LDIR_Name2[j-1] == 0x00))
			continue;
		if (ldr->LDIR_Name2[j] != 0x00)
			return -1;
		buf[i] = ldr->LDIR_Name2[j-1];
		if (--i < 0) return -1;
		a++;
	}
	// LDIR_Name1
	for (j = 9; j >= 0; j -= 2) {
		if ((ldr->LDIR_Name1[j] == 0xFF
				&& ldr->LDIR_Name1[j-1] == 0xFF)
				|| (ldr->LDIR_Name1[j] == 0x00
				&& ldr->LDIR_Name1[j-1] == 0x00))
			continue;
		if (ldr->LDIR_Name1[j] != 0x00)
			return -1;
		buf[i] = ldr->LDIR_Name1[j-1];
		if (--i < 0) return -1;
		a++;
	}

	return a;
}

static int give_filerecord(struct FAT32_DIR *dir,
					struct DIRRECORD32 *d,
					char *name)
{
	struct LONGDIRRECORD *ldr;
	int handling_lfn = 0;
	int nameplace;
	int tmp;
	uint8_t chksum;

	name[MAX_NAME_LENGTH - 1] = '\0';

	while (!give_dirrecord(dir, d)) {
		/* kansio käyty läpi */
		if (d->DIR_Name[0] == 0x00) {
			return -1;
		}

		/* Free entry */
		if (d->DIR_Name[0] == 0xE5) handling_lfn = 0;

		/* LFN record */
		else if ((d->DIR_Attr&ATTR_LONG_NAME_MASK)
						== ATTR_LONG_NAME) {
			ldr = (struct LONGDIRRECORD *)d;

			if (!handling_lfn) {
				if ((ldr->LDIR_Ord&LAST_LONG_ENTRY)
							!= LAST_LONG_ENTRY) {
					put_string("lfn orphan found\n");
				} else {
					handling_lfn = 1;
					nameplace = MAX_NAME_LENGTH - 2;
					chksum = ldr->LDIR_Chksum;
				}
			}

			if (handling_lfn && ldr->LDIR_Chksum != chksum) {
				put_string("lfn orphan found (chksum)\n");
				handling_lfn = 0;
			} else if (handling_lfn) {
				tmp = extract_lfname(ldr, name, nameplace);
				if (tmp < 0) {
					put_string("unicode/overlong name\n");
					handling_lfn = 0;
				} else
					nameplace -= tmp;
			}
		} else if (!(d->DIR_Name[0] == '.'
						&& d->DIR_Name[1] == ' ')) {

			if (handling_lfn && gen_chksum(d->DIR_Name) == chksum)
				strcpy(name, &name[nameplace + 1]);
			else
				conv83(d->DIR_Name, name);
			return 0;
		}
	}

	return -1;
}

static int open(struct FAT32_DIR *dir, const char *name, 
					open_t type,
					void *dest)
{
	struct DIRRECORD32 dr;
	struct FAT32_FILE *f = (struct FAT32_FILE*)dest;
	struct FAT32_DIR *d = (struct FAT32_DIR*)dest;
	char curname[MAX_NAME_LENGTH];
	uint32_t cluster;
	struct FAT32_DIR tmpdir;

	tmpdir.start_cluster = tmpdir.cur_cluster = dir->start_cluster;
	tmpdir.cur_sector = tmpdir.position = 0;

	while (!give_filerecord(&tmpdir, &dr, curname)) {

		if (!strcasecmp(name, curname)) {
			cluster = ((uint32_t)dr.DIR_FstClusHI
						<< 16) + dr.DIR_FstClusLO;

			if ((dr.DIR_Attr&(ATTR_DIRECTORY|ATTR_VOLUME_ID))
				== ATTR_DIRECTORY && type == DIRECTORY) {
				d->start_cluster = cluster < 2 ? 2: cluster;
				d->cur_cluster = d->start_cluster;
				d->cur_sector = d->position = 0;
				return 0;
			}

			if ((dr.DIR_Attr&(ATTR_DIRECTORY|ATTR_VOLUME_ID)) == 0
				&& type == FILE) {
				if (!f)
					return 0;

				f->start_cluster = cluster;
				f->cur_cluster = cluster;
				f->cur_sector = 0;
				f->cur_byte = 0;
				f->size = dr.DIR_FileSize;
				f->bytes_left = f->size;

				return 0;
			}

			return -1;
		}
	}

	return -1;
}

size_t fat32_read(void *d, size_t count, struct FAT32_FILE *f)
{
	size_t a;
	size_t total = 0;

	buffer_file_sector(f);

	if (f->bytes_left < count)
		count = f->bytes_left;

	while (count) {

		if (f->cur_byte == SECTOR_SIZE) {
			f->cur_byte = 0;
			f->cur_sector += 1;

			if (f->cur_sector == FAT32_S.sec_per_clus) {
				f->cur_sector = 0;
				f->cur_cluster = next_cluster(f->cur_cluster);
			}

			buffer_file_sector(f);
		}

		a = count < SECTOR_SIZE - f->cur_byte ? count :
				SECTOR_SIZE - f->cur_byte;
				
		memcpy(d, (void *)(FAT32_S.buf) + f->cur_byte,
			a);

		f->cur_byte += a;
		f->bytes_left -= a;
		count -= a;
		total += a;
		d += a;
	}

	return total;
}

int fat32_seek(size_t offset, struct FAT32_FILE *f)
{
	uint32_t clus_off;
	uint32_t sec_off;

	if (offset > f->size) return -1;

	f->bytes_left += f->cur_byte + f->cur_sector * SECTOR_SIZE;

	f->cur_byte = f->cur_sector = 0;

	if (f->size - f->bytes_left > offset) {
		f->cur_cluster = f->start_cluster;
		f->bytes_left = f->size;
	}
	else
		offset -= f->size - f->bytes_left;

	f->bytes_left -= offset;

	clus_off = offset / (FAT32_S.sec_per_clus * SECTOR_SIZE);
	sec_off = offset / SECTOR_SIZE - clus_off * FAT32_S.sec_per_clus;
	offset -= clus_off * FAT32_S.sec_per_clus * SECTOR_SIZE
		+ sec_off * SECTOR_SIZE;

	f->cur_sector = sec_off;
	f->cur_byte = offset;

	while (clus_off != 0) {
		f->cur_cluster = next_cluster(f->cur_cluster);
		clus_off--;
	}

	buffer_file_sector(f);

	return 0;
}

int fat32_chdir(struct FAT32_DIR *dir, const char *name)
{
	return open(dir, name, DIRECTORY, dir);
}

int fat32_opendir(const char *path, struct FAT32_DIR *dir)
{
	char buf[MAX_NAME_LENGTH];
	int i = 0, j = 0, depth = 0;

	while (path[j] != '\0')
		if (path[j++] == '/' && path[j] != '\0')
			depth++;

	dir->start_cluster = dir->cur_cluster = FAT32_S.rootdir_clusnum;
	dir->cur_sector = dir->position = 0;

	while (path[i] != '\0') {
		if (path[i++] == '/' && path[i] != '\0') {
			j = 0;
			while (path[i] != '/' && path[i] != '\0'
						&& j < MAX_NAME_LENGTH-1 ) {
				buf[j++] = path[i++];
			}
			buf[j] = '\0';
			if (fat32_chdir(dir, buf))
				return -1;
			depth--;
		}
	}

	return depth;
}

int fat32_fopen(struct FAT32_DIR *dir, const char *name, struct FAT32_FILE *f)
{
	return open(dir, name, FILE, f);
}

int fat32_readdir(struct FAT32_DIR *dir, char *name)
{
	struct DIRRECORD32 dr;

	if (give_filerecord(dir, &dr, name)) {
		return -1;
	}
	
	if ((dr.DIR_Attr&(ATTR_DIRECTORY|ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
		return 1;

	if ((dr.DIR_Attr&(ATTR_DIRECTORY|ATTR_VOLUME_ID)) == 0)
		return 0;

	return -1;
}
