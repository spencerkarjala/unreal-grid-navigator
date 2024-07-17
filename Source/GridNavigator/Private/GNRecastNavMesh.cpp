#include "GNRecastNavMesh.h"

#include "NavigationData.h"
#include "NavigationSystem.h"

#include <functional>
#include <unordered_map>

#include "MappingServer.h"
#include "NavMesh/PImplRecastNavMesh.h"

DECLARE_LOG_CATEGORY_CLASS(LogRGNRecastNavMesh, Log, All);

AGNRecastNavMesh::AGNRecastNavMesh(const FObjectInitializer& ObjectInitializer) : ARecastNavMesh(ObjectInitializer)
{
	this->FindPathImplementation = this->FindPath;
}

struct Point {
	int x, y;

	Point(int _x, int _y)
	{
		this->x = _x;
		this->y = _y;
	}

	Point(float _x, float _y)
	{
		this->x = round(_x);
		this->y = round(_y);
	}

	Point(double _x, double _y)
	{
		this->x = round(_x);
		this->y = round(_y);
	}

	explicit Point(const FVector& rhs)
	{
		this->x = round(rhs.X);
		this->y = round(rhs.Y);
	}
	
	const Point& operator=(const FVector& rhs) const
	{
		return Point(rhs);
	}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    // Euclidean distance for heuristic
    double DistanceTo(const Point& other) const {
        return std::hypot(x - other.x, y - other.y);
    }
};

namespace std {
    template <>
    struct hash<Point> {
        size_t operator()(const Point& p) const {
            return hash<int>()(p.x) ^ hash<int>()(p.y);
        }
    };
}

struct Node {
    Point position;
    Node* parent;
    double gCost; // Cost from start node to this node
    double hCost; // Heuristic cost from this node to end node
    double fCost() const { return gCost + hCost; } // Total cost
    
	Node(Point _position, Node* _parent, double _cost, double _heuristic) : position(_position), parent(_parent), gCost(_cost), hCost(_heuristic) {}
};

bool CompareNode(const Node* a, const Node* b) {
    return a->fCost() > b->fCost();
}

typedef std::function<double(const Point&, const Point&)> HeuristicFunction;
typedef std::function<bool(const FVector&, FVector&)> NavmeshLookupFunction;

class AStarPathfinding
{
public:
	static std::vector<Point> FindPath(const Point& start, const Point& end, const HeuristicFunction& heuristic, const NavmeshLookupFunction& navmesh_lookup)
	{
		const Point start1 = Point(round(start.x / 100.0) * 100.0, round(start.y / 100.0) * 100.0);
		const Point end1 = Point(round(end.x / 100.0) * 100.0, round(end.y / 100.0) * 100.0);
		std::priority_queue<Node*, std::vector<Node*>, decltype(&CompareNode)> openSet(CompareNode);
		std::unordered_map<Point, double> costSoFar; // Keeps track of the gCost for visited nodes
		openSet.emplace(new Node(start1, nullptr, 0, heuristic(start1, end1)));
		costSoFar[start1] = 0;

		int count = 0;

		Node* current = nullptr;
		bool IsPathingSuccessful = false;
		while (!openSet.empty()) {
			current = openSet.top();
			openSet.pop();

			if (current->position == end1) {
				IsPathingSuccessful = true;
				break;
			}

			const int size_grid = 100;

			std::vector<Point> neighbors = {
				{current->position.x + 1*size_grid, current->position.y + 0*size_grid},
				{current->position.x + 1*size_grid, current->position.y + 1*size_grid},
				{current->position.x - 0*size_grid, current->position.y + 1*size_grid},
				{current->position.x - 1*size_grid, current->position.y + 1*size_grid},
				{current->position.x - 1*size_grid, current->position.y - 0*size_grid},
				{current->position.x - 1*size_grid, current->position.y - 1*size_grid},
				{current->position.x + 0*size_grid, current->position.y - 1*size_grid},
				{current->position.x + 1*size_grid, current->position.y - 1*size_grid}
			};

			for (const auto& neighbor : neighbors) {
				double newCost = current->gCost + current->position.DistanceTo(neighbor);
				FVector neighbor_vec(neighbor.x, neighbor.y, 300.0);
				FVector waste(0.0);
				
				if (!navmesh_lookup(neighbor_vec, waste)) {
					continue;
				}
				if (!costSoFar.count(neighbor) || newCost < costSoFar[neighbor]) {
					costSoFar[neighbor] = newCost;
					double priority = newCost + heuristic(neighbor, end1);
					openSet.emplace(new Node(neighbor, current, newCost, heuristic(neighbor, end1)));
				}
			}
		}

		std::vector<Point> path;
		if (!IsPathingSuccessful) {
			return path;
		}

		while (current != nullptr) {
			path.push_back(current->position);
			current = current->parent;
		}
		std::reverse(path.begin(), path.end());

		while (!openSet.empty()) {
			delete openSet.top();
			openSet.pop();
		}

		return path;
	}
};

FPathFindingResult AGNRecastNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	DECLARE_CYCLE_STAT(TEXT("Grid Pathfinding"), STAT_Navigation_GridPathfinding, STATGROUP_Navigation);
	SCOPE_CYCLE_COUNTER(STAT_Navigation_GridPathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	FPathFindingResult Result(ENavigationQueryResult::Error);

	UE_LOG(LogRGNRecastNavMesh, Log, TEXT("Got cost limit of: '%0.2f'"), Query.CostLimit);

	const auto* Self = Cast<const ARecastNavMesh>(Query.NavData.Get());
	const auto* NavFilter = Query.QueryFilter.Get();

	if (!Self) {
		UE_LOG(LogRGNRecastNavMesh, Error, TEXT("Failed to retrieve reference to RecastNavMesh in FindPath"));
		return Result;
	}
	if (!NavFilter) {
		UE_LOG(LogRGNRecastNavMesh, Error, TEXT("Failed to retrieve reference to query filter in FindPath"));
		return Result;
	}
	
	auto* NavPath = Query.PathInstanceToFill.Get();
	auto* NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;

	if (NavMeshPath) {
		Result.Path = Query.PathInstanceToFill;
		NavMeshPath->ResetForRepath();
	}
	else {
		Result.Path = Self->CreatePathInstance<FNavMeshPath>(Query);
		NavPath = Result.Path.Get();
		NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;
	}

	if (!NavMeshPath) {
		UE_LOG(LogRGNRecastNavMesh, Error, TEXT("Somehow failed to instantiate destination navpath in FindPath; this should never happen"));
		return Result;
	}

	NavMeshPath->ApplyFlags(Query.NavDataFlags);

	// if travel distance is very small, return the corresponding very short path early
	const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
	if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero()) {
		Result.Path->GetPathPoints().Reset();
		Result.Path->GetPathPoints().Add(FNavPathPoint(AdjustedEndLocation));
		Result.Result = ENavigationQueryResult::Success;

		return Result;
	}

	const float DistanceBudget = Query.CostLimit;
	const FVector StartLocation = FMappingServer::RoundToGrid(Query.StartLocation);
	const FVector EndLocation   = FMappingServer::RoundToGrid(Query.EndLocation);

	const auto [Points, _] = FMappingServer::GetInstance().FindPath(StartLocation, EndLocation, DistanceBudget);

	if (Points.IsEmpty()) {
		Result = ENavigationQueryResult::Fail;
		return Result;
	}
	
	for (const FVector& Point : Points) {
		Result.Path->GetPathPoints().Add(FNavPathPoint(Point));
	}

	if (FVector::Distance(Points.Last(), EndLocation) > 0.1 && !Query.bAllowPartialPaths) {
		Result.Result = ENavigationQueryResult::Fail;
		return Result;
	}

	Result.Path->MarkReady();
	Result.Result = ENavigationQueryResult::Success;

	return Result;
}
