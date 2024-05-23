// SPDX-License-Identifier: GPL-2.0
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018 Redha Gouicem <redha.gouicem@lip6.fr>
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>

#include "ouichefs.h"
#include "bitmap.h"
#include "ouicheioctl.h"

/*
 * Map the buffer_head passed in argument with the iblock-th block of the file
 * represented by inode. If the requested block is not allocated and create is
 * true, allocate a new block on disk and map it.
 */
static int ouichefs_file_get_block(struct inode *inode, sector_t iblock,
				   struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index;
	int ret = 0, bno;

	/* If block number exceeds filesize, fail */
	if (iblock >= OUICHEFS_BLOCK_SIZE >> 2)
		return -EFBIG;

	/* Read index block from disk */
	bh_index = sb_bread(sb, ci->index_block);
	if (!bh_index)
		return -EIO;
	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	/*
	 * Check if iblock is already allocated. If not and create is true,
	 * allocate it. Else, get the physical block number.
	 */
	if (index->blocks[iblock] == 0) {
		if (!create) {
			ret = 0;
			goto brelse_index;
		}
		bno = get_free_block(sbi);
		if (!bno) {
			ret = -ENOSPC;
			goto brelse_index;
		}
		index->blocks[iblock] = bno;
	} else {
		bno = index->blocks[iblock];
	}

	/* Map the physical block to the given buffer_head */
	map_bh(bh_result, sb, bno);

brelse_index:
	brelse(bh_index);

	return ret;
}

/*
 * Called by the page cache to read a page from the physical disk and map it in
 * memory.
 */
static void ouichefs_readahead(struct readahead_control *rac)
{
	mpage_readahead(rac, ouichefs_file_get_block);
}

/*
 * Called by the page cache to write a dirty page to the physical disk (when
 * sync is called or when memory is needed).
 */
static int ouichefs_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, ouichefs_file_get_block, wbc);
}

/*
 * Called by the VFS when a write() syscall occurs on file before writing the
 * data in the page cache. This functions checks if the write will be able to
 * complete and allocates the necessary blocks through block_write_begin().
 */
static int ouichefs_write_begin(struct file *file,
				struct address_space *mapping, loff_t pos,
				unsigned int len, struct page **pagep,
				void **fsdata)
{
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(file->f_inode->i_sb);
	int err;
	uint32_t nr_allocs = 0;

	/* Check if the write can be completed (enough space?) */
	if (pos + len > OUICHEFS_MAX_FILESIZE)
		return -ENOSPC;
	nr_allocs = max(pos + len, file->f_inode->i_size) / OUICHEFS_BLOCK_SIZE;
	if (nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;
	if (nr_allocs > sbi->nr_free_blocks)
		return -ENOSPC;

	/* prepare the write */
	err = block_write_begin(mapping, pos, len, pagep,
				ouichefs_file_get_block);
	/* if this failed, reclaim newly allocated blocks */
	if (err < 0) {
		pr_err("%s:%d: newly allocated blocks reclaim not implemented yet\n",
		       __func__, __LINE__);
	}
	return err;
}

/*
 * Called by the VFS after writing data from a write() syscall to the page
 * cache. This functions updates inode metadata and truncates the file if
 * necessary.
 */
static int ouichefs_write_end(struct file *file, struct address_space *mapping,
			      loff_t pos, unsigned int len, unsigned int copied,
			      struct page *page, void *fsdata)
{
	int ret;
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;

	/* Complete the write() */
	ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
	if (ret < len) {
		pr_err("%s:%d: wrote less than asked... what do I do? nothing for now...\n",
		       __func__, __LINE__);
	} else {
		uint32_t nr_blocks_old = inode->i_blocks;

		/* Update inode metadata */
		inode->i_blocks = inode->i_size / OUICHEFS_BLOCK_SIZE + 2;
		inode->i_mtime = inode->i_ctime = current_time(inode);
		mark_inode_dirty(inode);

		/* If file is smaller than before, free unused blocks */
		if (nr_blocks_old > inode->i_blocks) {
			int i;
			struct buffer_head *bh_index;
			struct ouichefs_file_index_block *index;

			/* Free unused blocks from page cache */
			truncate_pagecache(inode, inode->i_size);

			/* Read index block to remove unused blocks */
			bh_index = sb_bread(sb, ci->index_block);
			if (!bh_index) {
				pr_err("failed truncating '%s'. we just lost %llu blocks\n",
				       file->f_path.dentry->d_name.name,
				       nr_blocks_old - inode->i_blocks);
				goto end;
			}
			index = (struct ouichefs_file_index_block *)
					bh_index->b_data;

			for (i = inode->i_blocks - 1; i < nr_blocks_old - 1;
			     i++) {
				put_block(OUICHEFS_SB(sb), index->blocks[i]);
				index->blocks[i] = 0;
			}
			mark_buffer_dirty(bh_index);
			brelse(bh_index);
		}
	}
end:
	return ret;
}

const struct address_space_operations ouichefs_aops = {
	.readahead = ouichefs_readahead,
	.writepage = ouichefs_writepage,
	.write_begin = ouichefs_write_begin,
	.write_end = ouichefs_write_end
};

static int ouichefs_open(struct inode *inode, struct file *file)
{
	bool wronly = (file->f_flags & O_WRONLY) != 0;
	bool rdwr = (file->f_flags & O_RDWR) != 0;
	bool trunc = (file->f_flags & O_TRUNC) != 0;

	if ((wronly || rdwr) && trunc && (inode->i_size != 0)) {
		struct super_block *sb = inode->i_sb;
		struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
		struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
		struct ouichefs_file_index_block *index;
		struct buffer_head *bh_index;
		sector_t iblock;

		/* Read index block from disk */
		bh_index = sb_bread(sb, ci->index_block);
		if (!bh_index)
			return -EIO;
		index = (struct ouichefs_file_index_block *)bh_index->b_data;

		for (iblock = 0; index->blocks[iblock] != 0; iblock++) {
			put_block(sbi, index->blocks[iblock]);
			index->blocks[iblock] = 0;
		}
		inode->i_size = 0;
		inode->i_blocks = 0;

		brelse(bh_index);
	}

	return 0;
}

/*
 * Read function for the ouichefs filesystem. This function allows to read data without
 * the use of page cache.
 */
static ssize_t ouichefs_read(struct file *file, char __user *data, size_t len, loff_t *pos)
{
	if (*pos >= file->f_inode->i_size)
		return 0;

	unsigned long to_be_copied = 0;
	unsigned long copied_to_user = 0;

	struct super_block *sb = file->f_inode->i_sb;
	sector_t iblock = *pos / OUICHEFS_BLOCK_SIZE;
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(file->f_inode);

	/* If block number exceeds filesize, fail */
	if (iblock >= OUICHEFS_BLOCK_SIZE >> 2)
		return -EFBIG;

	/* Read index block from disk */
	bh_index = sb_bread(sb, ci->index_block);
	if (!bh_index)
		return -EIO;
	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	/* Get the block number for the current iblock */
	int bno = index->blocks[iblock];

	if (bno == 0) {
		brelse(bh_index);
		return -EIO;
	}

	struct buffer_head *bh = sb_bread(sb, bno);

	if (!bh) {
		brelse(bh_index);
		return -EIO;
	}

	char *buffer = bh->b_data;

	// get data from the buffer from the current position
	buffer += *pos % OUICHEFS_BLOCK_SIZE;

	if (bh->b_size < len)
		to_be_copied = bh->b_size;
	else
		to_be_copied = len;

	copied_to_user = to_be_copied - copy_to_user(data, buffer, to_be_copied);

	*pos += copied_to_user;
	file->f_pos = *pos;

	brelse(bh);
	brelse(bh_index);

	return copied_to_user;
}

/*
 * Write function for the ouichefs filesystem. This function allows to write data without
 * the use of page cache.
 */
/*
static ssize_t ouichefs_write(struct file *file, const char __user *data, size_t len, loff_t *pos)
{
	struct inode *inode = file->f_inode;
	struct super_block *sb = inode->i_sb;
	// struct ouichefs_sb_info *sbi = OUICHEFS_SB(file->f_inode->i_sb);
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index, *bh;
	char *buffer;
	size_t to_be_written, written = 0;
	sector_t iblock;
	int bno;

	if (file->f_flags & O_APPEND)
		*pos = inode->i_size;

	if (*pos >= OUICHEFS_MAX_FILESIZE)
		return -EFBIG;

	bh_index = sb_bread(sb, ci->index_block);
	index = (struct ouichefs_file_index_block *)bh_index->b_data;
	iblock = *pos / OUICHEFS_BLOCK_SIZE;

	if (!bh_index)
		return -EIO;

	// Vérifier entre dernier bloc alloué et iblock si des blocs sont alloués, sinon les allouer
	for(int i=0; i<iblock; i++){
		if (index->blocks[i] == 0) {
			// Allouer un nouveau bloc
			bno = get_free_block(OUICHEFS_SB(sb));
			if (!bno) {
				brelse(bh_index);
				return -ENOSPC;
			}
			inode->i_blocks++;
			index->blocks[i] = bno;
			mark_buffer_dirty(bh_index);
			sync_dirty_buffer(bh_index);
		}
	}

	while (len > 0) {
		bh_index = sb_bread(sb, ci->index_block);
		index = (struct ouichefs_file_index_block *)bh_index->b_data;
		iblock = *pos / OUICHEFS_BLOCK_SIZE;
		// Vérifier si le bloc est déjà alloué
		if (index->blocks[iblock] == 0) {
			// Allouer un nouveau bloc
			bno = get_free_block(OUICHEFS_SB(sb));
			if (!bno) {
				brelse(bh_index);
				return -ENOSPC;
			}
			inode->i_blocks++;
			index->blocks[iblock] = bno;
			mark_buffer_dirty(bh_index);
			sync_dirty_buffer(bh_index);
		} else {
			bno = index->blocks[iblock];
		}

		// Lire ou initialiser le bloc de données
		bh = sb_bread(sb, bno);
		if (!bh) {
			brelse(bh_index);
			return -EIO;
		}
		buffer = bh->b_data;

		// Calculer la quantité de données à écrire dans ce bloc
		to_be_written = min(len, (size_t)(OUICHEFS_BLOCK_SIZE - (*pos % OUICHEFS_BLOCK_SIZE)));

		// Copier les données de l'utilisateur dans le bloc de données
		if (copy_from_user(buffer + (*pos % OUICHEFS_BLOCK_SIZE), data, to_be_written)) {
			brelse(bh);
			brelse(bh_index);
			return -EFAULT;
		}

		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
		brelse(bh);
		brelse(bh_index);

		*pos += to_be_written;
		data += to_be_written;
		len -= to_be_written;
		written += to_be_written;

		// Mettre à jour la taille du fichier si nécessaire
		if (*pos > inode->i_size) {
			inode->i_size = *pos;
			mark_inode_dirty(inode);
		}
	}

	return written;
}
*/

/*
* Find the correct block to write in the file
* Return the position in the block
*/
static inline loff_t ouichefs_find_block(struct inode *inode, loff_t * pos , sector_t * iblock, struct ouichefs_file_index_block *index)
{
	int total = 0;
	int i = 0;
	for(i=0; i<inode->i_blocks-1 ; i++){
		int tmp = total;
		int block_size = (index->blocks[i] >> 20);
		total += block_size;
		if(total > *pos){
			*iblock = i;
			return *pos - tmp;
		}
	}
	//ecriture dans un nouveau bloc : ibloc = 0 et la position dns le bloc = 0
	*iblock = 0;
	return 0;
}

/*
 * Inserts a block number into the index block of a file by adding it after iblock 
 * and keeping all the following blocks.
 */
static void ouichefs_insert_block_to_index(struct ouichefs_file_index_block *index, sector_t iblock, sector_t new_block)
{
	// Décaler les blocs suivants
	int i=iblock+1;
	sector_t tmp = index->blocks[i];
	index->blocks[i] = new_block;
	while(index->blocks[i] != 0){
		i++;
		sector_t tmp2 = index->blocks[i];
		index->blocks[i] = tmp;
		tmp = tmp2;
	}
	index->blocks[i++] = tmp;

	// Insérer le nouveau bloc
	index->blocks[iblock] = new_block;
}

/*
 * Write function for the ouichefs filesystem. This function allows to write data without
 * the use of page cache. This write function is the one that inserts data.
 */
static ssize_t ouichefs_write_insert(struct file *file, const char __user *data, size_t len, loff_t *pos)
{
	struct inode *inode = file->f_inode;
	struct super_block *sb = inode->i_sb;
	// struct ouichefs_sb_info *sbi = OUICHEFS_SB(file->f_inode->i_sb);
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index, *bh;
	char *buffer;
	size_t to_be_written, written = 0;
	sector_t iblock;
	int bno;

	if (file->f_flags & O_APPEND)
		*pos = inode->i_size;

	if (*pos >= OUICHEFS_MAX_FILESIZE)
		return -EFBIG;

	bh_index = sb_bread(sb, ci->index_block);
	index = (struct ouichefs_file_index_block *)bh_index->b_data;
	iblock = *pos / OUICHEFS_BLOCK_SIZE;

	if (!bh_index)
		return -EIO;

	//recuperer le bloc de donnees correspondant au pos
	//pos relative au bon bloc de donnée 
	loff_t pos_in_block = ouichefs_find_block(inode, pos, &iblock, index);

	// Vérifier entre dernier bloc alloué et iblock si des blocs sont alloués, sinon les allouer
	for(int i=0; i<iblock; i++){
		if (index->blocks[i] == 0) {
			// Allouer un nouveau bloc
			bno = get_free_block(OUICHEFS_SB(sb));
			if (!bno) {
				brelse(bh_index);
				return -ENOSPC;
			}
			inode->i_blocks++;
			index->blocks[i] = bno;
			mark_buffer_dirty(bh_index);
			sync_dirty_buffer(bh_index);
		}
	}

	while (len > 0) {
		bh_index = sb_bread(sb, ci->index_block);
		index = (struct ouichefs_file_index_block *)bh_index->b_data;

		// Vérifier si le bloc est déjà alloué
		if (index->blocks[iblock] == 0) {
			// Allouer un nouveau bloc
			bno = get_free_block(OUICHEFS_SB(sb));
			if (!bno) {
				brelse(bh_index);
				return -ENOSPC;
			}
			inode->i_blocks++;
			index->blocks[iblock] = bno;
			mark_buffer_dirty(bh_index);
			sync_dirty_buffer(bh_index);
		} else {
			bno = index->blocks[iblock];
		}

		// Lire ou initialiser le bloc de données
		bno = (index->blocks[iblock] & 0x000FFFFF);
		int size_block = (index->blocks[iblock] >> 20);
		bh = sb_bread(sb, bno);
		if (!bh) {
			brelse(bh_index);
			return -EIO;
		}
		buffer = bh->b_data;

		// Calculer la quantité de données à écrire dans ce bloc
		to_be_written = min(len, (size_t)(OUICHEFS_BLOCK_SIZE - (*pos % OUICHEFS_BLOCK_SIZE)));

		if(pos_in_block < size_block){
			//couper le block //allouer un nouveau bloc
			sector_t bisno = get_free_block(OUICHEFS_SB(sb));
			if (!bisno) {
				brelse(bh_index);
				return -ENOSPC;
			}
			inode->i_blocks++;
			mark_buffer_dirty(bh_index);
			sync_dirty_buffer(bh_index);

			struct buffer_head *bh_bis = sb_bread(sb, bisno);
			if (!bh_bis) {
				brelse(bh_index);
				return -EIO;
			}
			char *buffer_bis = buffer+pos_in_block;
			bh_bis->b_data = buffer_bis;

			mark_buffer_dirty(bh_bis);
			sync_dirty_buffer(bh_bis);
			brelse (bh_bis);

			int size_bis = (size_block-pos_in_block) << 20;
			bisno += size_bis;
			memset(buffer+pos_in_block, 0, size_bis);
			ouichefs_insert_block_to_index(index, iblock, bisno);

		}
		// Copier les données de l'utilisateur dans le bloc de données
		if (copy_from_user(buffer + (pos_in_block), data, to_be_written)) {
			brelse(bh);
			brelse(bh_index);
			return -EFAULT;
		}
		int size = pos_in_block+to_be_written;
		bno += size;
		// Mettre à jour la taille du fichier si nécessaire
		index->blocks[iblock] = bno;

		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
		brelse(bh);
		brelse(bh_index);

		*pos += to_be_written;
		data += to_be_written;
		len -= to_be_written;
		written += to_be_written;

		// Mettre à jour la taille du fichier si nécessaire
		if (*pos > inode->i_size) {
			inode->i_size = *pos;
			mark_inode_dirty(inode);
		}
	}

	return written;
}

/*
 * ioctl
 */
static long ouichefs_unlocked_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
	struct inode *inode = f->f_inode;
	struct ouichefs_file_index_block *index;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh_index;
	
	if(!inode) {
		pr_info("ouiche_ioctl: inode is NULL\n");
		return -ENOTTY;
	}

	switch (cmd) {
		case USED_BLKS:
			char used_blocks_char[64];
			snprintf(used_blocks_char, 64, "%llu", inode->i_blocks);
			if(copy_to_user((char *) arg, used_blocks_char, strlen(used_blocks_char))){
				pr_info("ouiche_ioctl: copy_to_user failed\n");
				return -EFAULT;
			}
			return 0;
		case PART_FILLED_BLKS:
		case INTERN_FRAG_WASTE:
			char retval[64];
			int part_filled_blocks = 0;
			int intern_frag_waste = 0;

			bh_index = sb_bread(sb, ci->index_block);

			if (!bh_index)
				return -EIO;

			index = (struct ouichefs_file_index_block *)bh_index->b_data;

			for (int i=0; i<inode->i_blocks-1; i++) {
				// La taille du bloc correspond aux 12 premiers bits du numéro de bloc
				int block_size = (index->blocks[i] >> 20);
				if(block_size < OUICHEFS_BLOCK_SIZE){
					intern_frag_waste += OUICHEFS_BLOCK_SIZE - block_size;
					part_filled_blocks++;
				}
			}

			brelse(bh_index);

			if(cmd == INTERN_FRAG_WASTE){
				snprintf(retval, 64, "%d", intern_frag_waste);
				if(copy_to_user((char *) arg, retval, strlen(retval))){
					pr_info("ouiche_ioctl: copy_to_user failed\n");
					return -EFAULT;
				}
			} else {
				snprintf(retval, 64, "%d", part_filled_blocks);
				if(copy_to_user((char *) arg, retval, strlen(retval))){
					pr_info("ouiche_ioctl: copy_to_user failed\n");
					return -EFAULT;
				}	
			}

			return 0;
		case USED_BLKS_INFO:
			bh_index = sb_bread(sb, ci->index_block);

			if (!bh_index)
				return -EIO;

			index = (struct ouichefs_file_index_block *)bh_index->b_data;

			for (int i=0; i<inode->i_blocks-1; i++) {
				// La taille du bloc correspond aux 12 premiers bits du numéro de bloc
				int block_size = (index->blocks[i] >> 20);
				int block_number = (index->blocks[i] & 0x000FFFFF);
				pr_info("Block %d: %d bytes\n", block_number, block_size);
			}

			brelse(bh_index);
			
			return 0;
		default:		
			pr_info("ouiche_ioctl: unknown command\n");
			return -ENOTTY;
	}
}

const struct file_operations ouichefs_file_ops = {
	.owner = THIS_MODULE,
	.open = ouichefs_open,
	.llseek = generic_file_llseek,
	.read_iter = generic_file_read_iter,
	.write_iter = generic_file_write_iter,
	.read = ouichefs_read,
	.write = ouichefs_write_insert,
	.unlocked_ioctl = ouichefs_unlocked_ioctl
};
