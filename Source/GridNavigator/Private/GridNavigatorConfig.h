#pragma once

class GridNavigatorConfig
{
public:
	static constexpr double MinHeightForValidNode = 125.0;
	static constexpr double GridSizeX = 100.0;
	static constexpr double GridSizeY = 100.0;
	static constexpr double GridSizeLayer = 25.0;
	
	static constexpr double MinEmptyLayersForValidNode = MinHeightForValidNode / GridSizeLayer;
};
