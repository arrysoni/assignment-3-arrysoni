#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

int check_mount = 0;
int write_permission = 0;

int mdadm_mount(void) // This code is directly taken from my Assignment 2
{

	// YOUR CODE GOES HERE

	if (check_mount == 1)
	{
		return -1;
	}

	uint32_t op = JBOD_MOUNT;
	int chk_status = jbod_operation(op << 12, NULL);

	if (chk_status == 0)
	{
		check_mount = 1;
		return 1;
	}

	return -1;
}

int mdadm_unmount(void) // This code is directly taken from my Assignment 2
{

	// YOUR CODE GOES HERE

	if (check_mount == 0)
	{
		return -1;
	}

	uint32_t op = JBOD_UNMOUNT;
	int chk_status = jbod_operation(op << 12, NULL);

	if (chk_status == 0)
	{
		check_mount = 0;
		return 1;
	}

	return -1;
}

int mdadm_write_permission(void)
{

	// YOUR CODE GOES HERE

	uint32_t op = JBOD_WRITE_PERMISSION;
	int write_status = jbod_operation(op << 12, NULL);

	if (write_status == 0)
	{
		write_permission = 1; // Permission granted to write
		return 0;			  // Success
	}

	return -1; // Failure
}

int mdadm_revoke_write_permission(void)
{

	// YOUR CODE GOES HERE

	uint32_t op = JBOD_REVOKE_WRITE_PERMISSION;
	int write_status = jbod_operation(op << 12, NULL);

	if (write_status == 0)
	{
		write_permission = 0; // Permission revoked to write
		return 0;			  // Success
	}

	return -1; // Failure
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)
{

	// YOUR CODE GOES HERE    // This code is directly taken from my Assignment 2

	// Check if mounted
	if (check_mount == 0)
	{
		return -3;
	}

	// Check for valid length and buffer
	if (read_len == 0 && read_buf == NULL)
	{
		return read_len;
	}

	if (read_len != 0 && read_buf == NULL)
	{
		return -4;
	}

	// Check for valid address and read length
	if ((start_addr + read_len) > (JBOD_NUM_DISKS * JBOD_DISK_SIZE))
	{
		return -1;
	}

	if (read_len > 1024)
	{
		return -2;
	}

	uint32_t remaining_len = read_len; // Number of bytes left to read
	uint32_t addr = start_addr;		   // Current address to read from
	uint8_t buffer_array[256];		   // Buffer to hold block data
	int bytes_read = 0;				   // Track total bytes read

	while (remaining_len > 0)
	{
		// Calculating the current disk, block, and position within block
		int current_Disk = addr / JBOD_DISK_SIZE;
		int current_Block = (addr / JBOD_BLOCK_SIZE) % JBOD_NUM_BLOCKS_PER_DISK;
		int current_PosInBlock = addr % JBOD_BLOCK_SIZE;

		// Checking if we're trying to read beyond the last disk
		if (current_Disk >= JBOD_NUM_DISKS)
		{
			return -1;
		}

		// Seeking to the correct disk
		uint32_t op_seek_disk = (JBOD_SEEK_TO_DISK << 12) | (current_Disk);
		if (jbod_operation(op_seek_disk, NULL) != 0)
		{
			return -1;
		}

		// Seeking to the correct block
		uint32_t op_seek_block = (JBOD_SEEK_TO_BLOCK << 12) | (current_Block << 4);
		if (jbod_operation(op_seek_block, NULL) != 0)
		{
			return -1;
		}

		// Reading the current block into buffer_array
		if (jbod_operation(JBOD_READ_BLOCK << 12, buffer_array) != 0)
		{
			return -1;
		}

		// Calculating how many bytes are left in the current block to read and the appropriate number of bytes needed to copy at a time
		int bytes_left_in_block = JBOD_BLOCK_SIZE - current_PosInBlock;
		int bytes_to_copy = (remaining_len < bytes_left_in_block) ? remaining_len : bytes_left_in_block;

		memcpy(read_buf + bytes_read, buffer_array + current_PosInBlock, bytes_to_copy);

		// Update for the next iteration
		addr += bytes_to_copy;
		remaining_len -= bytes_to_copy;
		bytes_read += bytes_to_copy;

		// If we've reached the end of the current block, move to the next block
		if (current_Block + 1 == JBOD_BLOCK_SIZE)
		{
			// Move to the next disk
			current_Disk += 1;
			current_Block = 0;		// Reset block to 0 when switching disks
			current_PosInBlock = 0; // Start from the beginning of the new block
		}
		else
		{
			// Move to the next block
			current_Block += 1;
			current_PosInBlock = 0; // Start from the beginning of the new block
		}
	}

	return read_len;
}

int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf)
{

	// YOUR CODE GOES HERE

	// Check if system is mounted
	if (check_mount == 0)
	{
		return -3;
	}

	// Check if write permission is granted
	if (write_permission == 0)
	{
		return -5;
	}

	// Check for valid length and buffer
	if (write_len == 0 && write_buf == NULL)
	{
		return write_len;
	}
	if (write_len != 0 && write_buf == NULL)
	{
		return -4;
	}

	// Check for write length bounds
	if (write_len > 1024)
	{
		return -2;
	}

	// Check for address bounds
	if ((start_addr + write_len) > (JBOD_NUM_DISKS * JBOD_DISK_SIZE))
	{
		return -1;
	}

	uint32_t addr = start_addr; // Current address to write to
	uint8_t buffer_array[256];	// Buffer to hold block data
	int bytes_written = 0;

	while (bytes_written < write_len)
	{
		// Calculate disk, block, and offset within the block
		int current_Disk = addr / JBOD_DISK_SIZE;
		int current_Block = (addr / JBOD_BLOCK_SIZE) % JBOD_NUM_BLOCKS_PER_DISK;
		int current_PosInBlock = addr % JBOD_DISK_SIZE % JBOD_BLOCK_SIZE;

		// Seek to the correct disk
		uint32_t op_seek_disk = (JBOD_SEEK_TO_DISK << 12) | current_Disk;
		if (jbod_operation(op_seek_disk, NULL) != 0)
		{
			return -1;
		}

		// Seek to the correct block
		uint32_t op_seek_block = (JBOD_SEEK_TO_BLOCK << 12) | (current_Block << 4);
		if (jbod_operation(op_seek_block, NULL) != 0)
		{
			return -1;
		}

		// Read the current block to avoid overwriting data outside the write range
		if (jbod_operation(JBOD_READ_BLOCK << 12, buffer_array) != 0)
		{
			return -1;
		}

		// Determine how many bytes to write to the current block
		int bytes_left_in_block = JBOD_BLOCK_SIZE - current_PosInBlock;
		bytes_left_in_block = bytes_left_in_block > write_len - bytes_written ? write_len - bytes_written : bytes_left_in_block;
		// Copy data from write_buf to buffer_array, starting at current_PosInBlock
		memcpy(buffer_array + current_PosInBlock, write_buf + bytes_written, bytes_left_in_block);

		if (jbod_operation(JBOD_WRITE_BLOCK << 12, buffer_array) != 0)
		{
			return -1;
		}

		addr += bytes_left_in_block;
		bytes_written += bytes_left_in_block;
	}

	return write_len;
}
