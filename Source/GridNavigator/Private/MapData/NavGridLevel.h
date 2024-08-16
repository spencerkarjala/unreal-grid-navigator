#pragma once
#include "NavGridBlock.h"

namespace NavGrid
{
	class FLevel
	{
	public:
		/**
		 * @brief Checks whether a navigation block exists for a particular ID.
		 *
		 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
		 * @return \c true if level has a block corresponding to ID; \c false otherwise
		 */
		FORCEINLINE bool HasBlockWithID(const uint32 ID) const { return Blocks.Contains(ID); }

		/**
		 * @brief Returns a navigation block associated with a particular ID.
		 * 
		 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
		 * @return The \c FBlock associated with ID
		 */
		FORCEINLINE FBlock const* GetBlock(const uint32 ID) const { return Blocks.Find(ID); }

		/**
		 * @brief Returns a navigation block associated with a particular ID.
		 * 
		 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
		 * @return The \c FBlock associated with ID
		 *
		 * @note Does not check whether the entry exists; check with HasBlockWithID first.
		 */
		FORCEINLINE const FBlock& GetBlockChecked(const uint32 ID) const { return Blocks.FindChecked(ID); }

		/**
		 * @brief Adds a new navigation block block associated with a particular ID.
		 * 
		 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
		 * @param Block New block data associated with UniqueID
		 */
		FORCEINLINE void AddBlock(const uint32 ID, FBlock&& Block) { Blocks[ID] = Block; }

		/**
		 * @brief Updates the data for a navigation block associated with a particular ID.
		 * 
		 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
		 * @param Block New block data associated with UniqueID
		 */
		FORCEINLINE void UpdateBlock(const uint32 ID, FBlock&& Block) { Blocks[ID] = Block; }

	private:
		TMap<uint32, FBlock> Blocks;
	};
}