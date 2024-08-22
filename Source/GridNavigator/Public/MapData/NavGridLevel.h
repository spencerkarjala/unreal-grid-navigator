#pragma once
#include "NavGridBlock.h"

#include "CoreMinimal.h"
#include "NavGridLevel.generated.h"

USTRUCT()
struct FNavGridLevel
{
	GENERATED_BODY()
	
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
	 * @return The \c FNavGridBlock associated with ID
	 */
	FORCEINLINE FNavGridBlock const* GetBlock(const uint32 ID) const { return Blocks.Find(ID); }

	/**
	 * @brief Returns a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @return The \c FNavGridBlock associated with ID
	 *
	 * @note Does not check whether the entry exists; check with HasBlockWithID first.
	 */
	FORCEINLINE const FNavGridBlock& GetBlockChecked(const uint32 ID) const { return Blocks.FindChecked(ID); }

	/**
	 * @brief Adds a new navigation block block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @param Block New block data associated with UniqueID
	 */
	FORCEINLINE void AddBlock(const uint32 ID, FNavGridBlock&& Block) { Blocks[ID] = Block; }

	/**
	 * @brief Updates the data for a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @param Block New block data associated with UniqueID
	 */
	FORCEINLINE void UpdateBlock(const uint32 ID, FNavGridBlock&& Block) { Blocks[ID] = Block; }

	/**
	 * @brief Retrieves a string that summarizes the data currently stored by the level.
	 * 
	 * @return A string summarizing the level's data content
	 */
	FString ToString() const;

private:
	TMap<uint32, FNavGridBlock> Blocks;
};