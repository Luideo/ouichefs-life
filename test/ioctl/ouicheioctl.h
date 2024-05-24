#pragma once

#define IO_MAGIC		'v'

/*
 * USED_BLKS: return the number of used blocks
 */
#define USED_BLKS		_IOR(IO_MAGIC, 0, char *)

/*
 * PART_FILLED_BLKS: return the number of partially filled blocks
 */
#define PART_FILLED_BLKS	_IOR(IO_MAGIC, 1, char *)

/*
 * INTERN_FRAG_WASTE: return the number of bytes wasted due to internal fragmentation
 */
#define INTERN_FRAG_WASTE	_IOR(IO_MAGIC, 2, char *)

/*
 * USED_BLKS_INFO: returns the list of all used blocks with their number and effective size
 */
#define USED_BLKS_INFO		_IOR(IO_MAGIC, 3, char *)

/*
 * DEFRAG: defragment the file
 */
#define DEFRAG			_IO(IO_MAGIC, 4)