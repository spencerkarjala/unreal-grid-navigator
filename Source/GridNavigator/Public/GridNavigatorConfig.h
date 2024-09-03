#pragma once

class GridNavigatorConfig
{
public:
	static constexpr double MinHeightForValidNode = 125.0;
	static constexpr double GridSizeX = 100.0;
	static constexpr double GridSizeY = 100.0;
	static constexpr double GridSizeZ = 25.0;
	
	static constexpr double MinEmptyLayersForValidNode = MinHeightForValidNode / GridSizeZ;

	static constexpr float ASSUMED_GRID_SPACING = 100.0;

	static FIntVector2 WorldToGridIndex(const FVector2f& WorldCoord)
	{
		return FIntVector2(
			FMath::RoundToInt(WorldCoord.X / static_cast<float>(GridNavigatorConfig::GridSizeX)),
			FMath::RoundToInt(WorldCoord.Y / static_cast<float>(GridNavigatorConfig::GridSizeY))
		);
	}

	static FIntVector3 WorldToGridIndex(const FVector& WorldCoord)
	{
		return FIntVector3(
			FMath::RoundToInt(WorldCoord.X / static_cast<float>(GridNavigatorConfig::GridSizeX)),
			FMath::RoundToInt(WorldCoord.Y / static_cast<float>(GridNavigatorConfig::GridSizeY)),
			FMath::RoundToInt(WorldCoord.Z / static_cast<float>(GridNavigatorConfig::GridSizeZ))
		);
	}

	static FVector2f GridIndexToWorld(const FIntVector2& IndexCoord)
	{
		return FVector2f(
			static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
			static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY)
		);
	}

	static FVector2f GridIndexToWorld(const FVector2f& IndexCoord)
	{
		return FVector2f(
			static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
			static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY)
		);
	}

	static FVector GridIndexToWorld(const FIntVector3& IndexCoord)
	{
		return FVector(
			static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
			static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY),
			static_cast<float>(IndexCoord.Z) * static_cast<float>(GridNavigatorConfig::GridSizeZ)
		);
	}

	static FVector GridIndexToWorld(const FVector& IndexCoord)
	{
		return FVector(
			static_cast<float>(IndexCoord.X) * static_cast<float>(GridNavigatorConfig::GridSizeX),
			static_cast<float>(IndexCoord.Y) * static_cast<float>(GridNavigatorConfig::GridSizeY),
			static_cast<float>(IndexCoord.Z) * static_cast<float>(GridNavigatorConfig::GridSizeZ)
		);
	}
	
	static FVector RoundToGrid(const FVector& Value) 
	{
		return FVector(
			round(Value.X / 100.0) * 100.00,
			round(Value.Y / 100.0) * 100.00,
			round(Value.Z / 25.0) * 25.0
		);
	}
	
	static FVector TruncToGrid(const FVector& Value)
	{
		return FVector(
			floor(Value.X / 100.0) * 100.0,
			floor(Value.Y / 100.0) * 100.0,
			floor(Value.Z / 25.0) * 25.0
		);
	}
};
