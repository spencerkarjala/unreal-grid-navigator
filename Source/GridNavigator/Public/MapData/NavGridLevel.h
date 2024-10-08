#pragma once
#include "NavGridBlock.h"

#include "CoreMinimal.h"
#include "MapData/NavGridAdjacencyList.h"
#include "NavGridLevel.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FNavGridLevel
{
	GENERATED_BODY()
	
	/**
	 * @brief Checks whether a navigation block exists for a particular ID.
	 *
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @return \c true if level has a block corresponding to ID; \c false otherwise
	 */
	FORCEINLINE bool HasBlockWithID(const uint32 ID) const
	{
		return Blocks.Contains(ID);
	}

	/**
	 * @brief Returns a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @return The \c FNavGridBlock associated with ID
	 */
	FORCEINLINE FNavGridBlock const* GetBlock(const uint32 ID) const
	{
		return Blocks.Find(ID);
	}

	/**
	 * @brief Returns all of the currently-stored navigation data blocks.
	 * 
	 * @return An array with all of the current navigation data blocks
	 */
	FORCEINLINE TArray<FNavGridBlock> GetBlocks() const
	{
		TArray<FNavGridBlock> Result;
		Blocks.GenerateValueArray(Result);
		return Result;
	}

	/**
	 * @brief Returns a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @return The \c FNavGridBlock associated with ID
	 *
	 * @note Does not check whether the entry exists; check with HasBlockWithID first.
	 */
	FORCEINLINE const FNavGridBlock& GetBlockChecked(const uint32 ID) const
	{
		return Blocks.FindChecked(ID);
	}

	/**
	 * @brief Adds a new navigation block block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @param Block New block data associated with UniqueID
	 */
	FORCEINLINE void AddBlock(const uint32 ID, FNavGridBlock&& Block)
	{
		Blocks.Add(ID, Block);
	}

	/**
	 * @brief Updates the data for a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @param Block New block data associated with UniqueID
	 */
	FORCEINLINE void UpdateBlock(const uint32 ID, FNavGridBlock&& Block)
	{
		Blocks[ID] = Block;
	}

	/**
	 * @brief Removes the data for a navigation block associated with a particular ID.
	 * 
	 * @param ID UniqueID (from FNavigationBounds) that identifies the block/nav bound
	 * @note Assumes that ID is present in the list of blocks.
	 */
	FORCEINLINE void RemoveBlock(const uint32 ID)
	{
		Blocks.Remove(ID);
	}

	/**
	 * @brief Retrieves a string that summarizes the data currently stored by the level.
	 * 
	 * @return A string summarizing the level's data content
	 */
	FString ToString() const;

	/**
	 * @brief Retrieves the bounding box for the stored navigation data.
	 * 
	 * @return The smallest \c FBox that contains all navigation data
	 */
	FBox GetBounds() const;

	TMap<uint32, FNavGridBlock> Blocks;
	FNavGridAdjacencyList Map;
};
