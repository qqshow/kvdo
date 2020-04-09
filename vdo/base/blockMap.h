/*
 * Copyright (c) 2020 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 *
 * $Id: //eng/linux-vdo/src/c++/vdo/base/blockMap.h#15 $
 */

#ifndef BLOCK_MAP_H
#define BLOCK_MAP_H

#include "adminState.h"
#include "blockMapEntry.h"
#include "completion.h"
#include "fixedLayout.h"
#include "statistics.h"
#include "types.h"

/**
 * Create a block map.
 *
 * @param [in]  logical_blocks     The number of logical blocks for the VDO
 * @param [in]  thread_config      The thread configuration of the VDO
 * @param [in]  flat_page_count    The number of flat pages
 * @param [in]  root_origin        The absolute PBN of the first root page
 * @param [in]  root_count         The number of tree roots
 * @param [out] map_ptr            The pointer to hold the new block map
 *
 * @return VDO_SUCCESS or an error code
 **/
int make_block_map(BlockCount logical_blocks,
		   const struct thread_config *thread_config,
		   BlockCount flat_page_count,
		   PhysicalBlockNumber root_origin,
		   BlockCount root_count,
		   struct block_map **map_ptr)
	__attribute__((warn_unused_result));

/**
 * Quiesce all block map I/O, possibly writing out all dirty metadata.
 *
 * @param map        The block map to drain
 * @param operation  The type of drain to perform
 * @param parent     The completion to notify when the drain is complete
 **/
void drain_block_map(struct block_map *map,
		     AdminStateCode operation,
		     struct vdo_completion *parent);

/**
 * Resume I/O for a quiescent block map.
 *
 * @param map     The block map to resume
 * @param parent  The completion to notify when the resume is complete
 **/
void resume_block_map(struct block_map *map, struct vdo_completion *parent);

/**
 * Prepare to grow the block map by allocating an expanded collection of trees.
 *
 * @param map                 The block map to grow
 * @param new_logical_blocks  The new logical size of the VDO
 *
 * @return VDO_SUCCESS or an error
 **/
int prepare_to_grow_block_map(struct block_map *map,
			      BlockCount new_logical_blocks)
	__attribute__((warn_unused_result));

/**
 * Get the logical size to which this block map is prepared to grow.
 *
 * @param map  The block map
 *
 * @return The new number of entries the block map will be grown to or 0 if
 *         the block map is not prepared to grow
 **/
BlockCount get_new_entry_count(struct block_map *map)
	__attribute__((warn_unused_result));

/**
 * Grow a block map on which prepare_to_grow_block_map() has already been
 *called.
 *
 * @param map     The block map to grow
 * @param parent  The object to notify when the growth is complete
 **/
void grow_block_map(struct block_map *map, struct vdo_completion *parent);

/**
 * Abandon any preparations which were made to grow this block map.
 *
 * @param map  The map which won't be grown
 **/
void abandon_block_map_growth(struct block_map *map);

/**
 * Decode the state of a block map saved in a buffer, without creating page
 * caches.
 *
 * @param [in]  buffer          A buffer containing the super block state
 * @param [in]  logical_blocks  The number of logical blocks for the VDO
 * @param [in]  thread_config   The thread configuration of the VDO
 * @param [out] map_ptr         The pointer to hold the new block map
 *
 * @return VDO_SUCCESS or an error code
 **/
int decode_block_map(struct buffer *buffer,
		     BlockCount logical_blocks,
		     const struct thread_config *thread_config,
		     struct block_map **map_ptr)
	__attribute__((warn_unused_result));

/**
 * Create a block map from the saved state of a Sodium block map, and do any
 * necessary upgrade work.
 *
 * @param [in]  buffer          A buffer containing the super block state
 * @param [in]  logical_blocks  The number of logical blocks for the VDO
 * @param [in]  thread_config   The thread configuration of the VDO
 * @param [out] map_ptr         The pointer to hold the new block map
 *
 * @return VDO_SUCCESS or an error code
 **/
int decode_sodium_block_map(struct buffer *buffer,
			    BlockCount logical_blocks,
			    const struct thread_config *thread_config,
			    struct block_map **map_ptr)
	__attribute__((warn_unused_result));

/**
 * Allocate the page caches for a block map.
 *
 * @param map                 The block map needing caches.
 * @param layer               The physical layer for the cache
 * @param read_only_notifier  The read only mode context
 * @param journal             The recovery journal (may be NULL)
 * @param nonce               The nonce to distinguish initialized pages
 * @param cache_size          The block map cache size, in pages
 * @param maximum_age         The number of journal blocks before a dirtied page
 *                            is considered old and must be written out
 *
 * @return VDO_SUCCESS or an error code
 **/
int make_block_map_caches(struct block_map *map,
			  PhysicalLayer *layer,
			  struct read_only_notifier *read_only_notifier,
			  struct recovery_journal *journal,
			  Nonce nonce,
			  PageCount cache_size,
			  BlockCount maximum_age)
	__attribute__((warn_unused_result));

/**
 * Free a block map and null out the reference to it.
 *
 * @param map_ptr  A pointer to the block map to free
 **/
void free_block_map(struct block_map **map_ptr);

/**
 * Get the size of the encoded state of a block map.
 *
 * @return The encoded size of the map's state
 **/
size_t get_block_map_encoded_size(void) __attribute__((warn_unused_result));

/**
 * Encode the state of a block map into a buffer.
 *
 * @param map     The block map to encode
 * @param buffer  The buffer to encode into
 *
 * @return UDS_SUCCESS or an error
 **/
int encode_block_map(const struct block_map *map, struct buffer *buffer)
	__attribute__((warn_unused_result));

/**
 * Obtain any necessary state from the recovery journal that is needed for
 * normal block map operation.
 *
 * @param map      The map in question
 * @param journal  The journal to initialize from
 **/
void initialize_block_map_from_journal(struct block_map *map,
				       struct recovery_journal *journal);

/**
 * Get the portion of the block map for a given logical zone.
 *
 * @param map          The map
 * @param zone_number  The number of the zone
 *
 * @return The requested block map zone
 **/
struct block_map_zone *get_block_map_zone(struct block_map *map,
					  ZoneCount zone_number)
	__attribute__((warn_unused_result));

/**
 * Compute the logical zone on which the entry for a data_vio
 * resides
 *
 * @param data_vio  The data_vio
 *
 * @return The logical zone number for the data_vio
 **/
ZoneCount compute_logical_zone(struct data_vio *data_vio);

/**
 * Compute the block map slot in which the block map entry for a data_vio
 * resides, and cache that number in the data_vio.
 *
 * @param data_vio  The data_vio
 * @param callback  The function to call once the slot has been found
 * @param thread_id The thread on which to run the callback
 **/
void find_block_map_slot_async(struct data_vio *data_vio,
			       vdo_action *callback,
			       ThreadID thread_id);

/**
 * Get number of block map pages at predetermined locations.
 *
 * @param map  The block map
 *
 * @return The number of fixed pages used by the map
 **/
PageCount get_number_of_fixed_block_map_pages(const struct block_map *map)
	__attribute__((warn_unused_result));

/**
 * Get number of block map entries.
 *
 * @param map  The block map
 *
 * @return The number of entries stored in the map
 **/
BlockCount get_number_of_block_map_entries(const struct block_map *map)
	__attribute__((warn_unused_result));

/**
 * Notify the block map that the recovery journal has finished a new block.
 * This method must be called from the journal zone thread.
 *
 * @param map                    The block map
 * @param recovery_block_number  The sequence number of the finished recovery
 *                               journal block
 **/
void advance_block_map_era(struct block_map *map,
			   SequenceNumber recovery_block_number);

/**
 * Get the block number of the physical block containing the data for the
 * specified logical block number. All blocks are mapped to physical block
 * zero by default, which is conventionally the zero block.
 *
 * @param data_vio  The data_vio of the block to map
 **/
void get_mapped_block_async(struct data_vio *data_vio);

/**
 * Associate the logical block number for a block represented by a data_vio
 * with the physical block number in its newMapped field.
 *
 * @param data_vio  The data_vio of the block to map
 **/
void put_mapped_block_async(struct data_vio *data_vio);

/**
 * Get the stats for the block map page cache.
 *
 * @param map  The block map containing the cache
 *
 * @return The block map statistics
 **/
struct block_map_statistics get_block_map_statistics(struct block_map *map)
	__attribute__((warn_unused_result));

#endif // BLOCK_MAP_H
