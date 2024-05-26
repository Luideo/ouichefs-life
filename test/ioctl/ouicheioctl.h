/* SPDX-License-Identifier: GPL-2.0 */
#pragma once

#define IO_MAGIC		'v'

/*
 * USED_BLKS: return the number of used blocks
 */
#define INFO			_IO(IO_MAGIC, 0)

/*
 * DEFRAG: defragment the file
 */
#define DEFRAG			_IO(IO_MAGIC, 1)

/*
 * SWITCH_MODE: switch the read/write mode from normal to insert and vice versa
 */
#define SWITCH_MODE		_IO(IO_MAGIC, 2)

/*
 * DISPLAY_MODE: diplay the current read/write mode
 */
#define DISPLAY_MODE		_IOR(IO_MAGIC, 3, char *)