// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGeneratorII.h"

#include "Components/InstancedStaticMeshComponent.h"


ACityGeneratorII::ACityGeneratorII()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ACityGeneratorII::BeginPlay()
{
    Super::BeginPlay();
    Generate();
}


void ACityGeneratorII::Generate()
{
    Clear();
    GridSize = FMath::Max(GridSize, 5);

    InitGrid();
    PlaceRoads();

    FRandomStream Rand(Seed);
    PlaceBuildings(Rand);
    SpawnInstances();
}

void ACityGeneratorII::Clear()
{
    ClearISMCs();
    Grid.Empty();
    BuildingID.Empty();
    MergedEast.Empty();
}


void ACityGeneratorII::InitGrid()
{
    const int Total = GridSize * GridSize;
    Grid.Init(ETileType::Empty, Total);
    BuildingID.Init(-1, Total);
    MergedEast.Init(false, Total);
}


void ACityGeneratorII::PlaceRoads()
{
    // perimeter
    for (int i = 0; i < GridSize; ++i)
    {
        SetRoad(i, 0);
        SetRoad(i, GridSize - 1);
        SetRoad(0, i);
        SetRoad(GridSize - 1, i);
    }

    // inner (plain grid)
    //for (int Y = 1; Y < GridSize - 1; ++Y)
    //{
    //    for (int X = 1; X < GridSize - 1; ++X)
    //    {
    //        // road when either X or Y is on a block boundary
    //        if (X % BlockSize == 0 || Y % BlockSize == 0)
    //            SetRoad(X, Y);
    //    }
    //}

    // Recursively subdivide the interior
    SubdivideBlock(1, 1, GridSize - 2, GridSize - 2, 0);
}

void ACityGeneratorII::SubdivideBlock(int X0, int Y0, int X1, int Y1, int Depth)
{
    int Width = X1 - X0;
    int Height = Y1 - Y0;

    // Stop subdividing when the block is too small
    if (Width < MinBlockSize && Height < MinBlockSize)
        return;

    // Bias: prefer splitting the longer axis, but sometimes flip randomly
    bool bSplitHorizontal = (Height > Width);
    if (FMath::RandBool() && Depth < MaxDepth)
        bSplitHorizontal = !bSplitHorizontal;

    if (bSplitHorizontal && Height >= MinBlockSize)
    {
        // Pick a split Y somewhere in the middle third (avoids tiny slivers)
        int Margin = FMath::Max(1, Height / 4);
        int SplitY = FMath::RandRange(Y0 + Margin, Y1 - Margin);

        for (int X = X0; X <= X1; ++X)
            SetRoad(X, SplitY);

        SubdivideBlock(X0, Y0, X1, SplitY - 1, Depth + 1);
        SubdivideBlock(X0, SplitY + 1, X1, Y1, Depth + 1);
    }
    else if (!bSplitHorizontal && Width >= MinBlockSize)
    {
        int Margin = FMath::Max(1, Width / 4);
        int SplitX = FMath::RandRange(X0 + Margin, X1 - Margin);

        for (int Y = Y0; Y <= Y1; ++Y)
            SetRoad(SplitX, Y);

        SubdivideBlock(X0, Y0, SplitX - 1, Y1, Depth + 1);
        SubdivideBlock(SplitX + 1, Y0, X1, Y1, Depth + 1);
    }
}

void ACityGeneratorII::PlaceBuildings(FRandomStream& Rand)
{
    if (BuildingMeshes.Num() == 0)
        return;

    const int NumMeshes = BuildingMeshes.Num();

    // assign building to non road tiles
    int index;
    for (int Y = 0; Y < GridSize; ++Y)
    {
        for (int X = 0; X < GridSize; ++X)
        {
            if (Grid[index = Index(X, Y)] == ETileType::Empty)
            {
                Grid[index] = ETileType::Building;
                BuildingID[index] = Rand.RandRange(0, NumMeshes - 1);
            }
        }
    }

    //// Pass 2 – try to merge adjacent building tiles into 2×1 pairs (east–west)
    ////          A merged pair re-uses the west tile's mesh scaled 2× on X.
    //for (int Y = 0; Y < GridSize; ++Y)
    //{
    //    for (int X = 0; X < GridSize - 1; ++X)
    //    {
    //        const int IdxA = Index(X, Y);
    //        const int IdxB = Index(X + 1, Y);

    //        if (Grid[IdxA] == ETileType::Building && Grid[IdxB] == ETileType::Building &&
    //            !MergedEast[IdxA] &&           // A not already used as left half
    //            BuildingID[IdxB] != -1 &&       // B is free
    //            Rand.FRand() < MergeProbability)
    //        {
    //            // Mark A as the anchor (left half) of a 2×1 building
    //            MergedEast[IdxA] = true;
    //            BuildingID[IdxB] = -1;         // B is swallowed – skip during spawn
    //            // Keep BuildingID[IdxA] as the mesh to use
    //        }
    //    }
    //}
}


void ACityGeneratorII::SpawnInstances()
{
    for (int Y = 0; Y < GridSize; ++Y)
    {
        for (int X = 0; X < GridSize; ++X)
        {
            const ETileType Type = Grid[Index(X, Y)];

            if (Type == ETileType::Road)
            {
                float Yaw = 0.f;
                UStaticMesh* Mesh = GetRelevantMeshForRoad(GetRelevantRoadTile(X, Y), Yaw);
                if (!Mesh)
                    continue;

                UInstancedStaticMeshComponent* ISMC = GetOrCreateISMC(Mesh);
                ISMC->AddInstance(MakeTransformForTile(X, Y, Yaw));
            }
            else if (Type == ETileType::Building)
            {
                const int MeshIdx = BuildingID[Index(X, Y)];
                if (MeshIdx < 0)
                    continue; // swallowed by merge

                if (!BuildingMeshes.IsValidIndex(MeshIdx))
                    continue;

                UStaticMesh* Mesh = BuildingMeshes[MeshIdx];
                if (!Mesh)
                    continue;

                UInstancedStaticMeshComponent* ISMC = GetOrCreateISMC(Mesh);

                // 2×1 merged building: scale X by 2, offset half a tile east
                if (MergedEast[Index(X, Y)])
                    ISMC->AddInstance(MakeTransformForTile(X, Y, 0.f, 2.f));
                else
                    ISMC->AddInstance(MakeTransformForTile(X, Y, 0.f, 1.f));
            }
        }
    }
}


UInstancedStaticMeshComponent* ACityGeneratorII::GetOrCreateISMC(UStaticMesh* Mesh)
{
    if (UInstancedStaticMeshComponent** Found = ISMCMap.Find(Mesh))
        return *Found;

    UInstancedStaticMeshComponent* ISMC = NewObject<UInstancedStaticMeshComponent>(this);
    ISMC->SetStaticMesh(Mesh);
    ISMC->SetMobility(EComponentMobility::Static);
    ISMC->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    ISMC->RegisterComponent();

    ISMCMap.Add(Mesh, ISMC);
    return ISMC;
}

void ACityGeneratorII::ClearISMCs()
{
    for (auto& Pair : ISMCMap)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
            Pair.Value->DestroyComponent();
        }
    }
    ISMCMap.Empty();
}


FTransform ACityGeneratorII::MakeTransformForTile(int X, int Y, float Yaw, float ScaleX) const
{
    FVector Loc(
        (X + ScaleX * 0.5f) * TileSize,   // shift right by half for 2×1
        (Y + 0.5f) * TileSize,
        0.f
    );

    FRotator Rot(0.f, Yaw, 0.f);
    FVector  Scale(ScaleX, 1.f, 1.f);

    return FTransform(Rot, Loc + GetActorLocation(), Scale);
}


bool ACityGeneratorII::IsRoad(int X, int Y) const
{
    if (X < 0 || X >= GridSize)
        return false;
    if (Y < 0 || Y >= GridSize)
        return false;

    return Grid[Index(X, Y)] == ETileType::Road;
}

ERoadType ACityGeneratorII::GetRelevantRoadTile(int X, int Y) const
{
    const bool NorthTileIsRoad = IsRoad(X, Y + 1);
    const bool SouthTileIsRoad = IsRoad(X, Y - 1);
    const bool EastTileIsRoad = IsRoad(X + 1, Y);
    const bool WestTileIsRoad = IsRoad(X - 1, Y);
    const int Count = NorthTileIsRoad + SouthTileIsRoad + WestTileIsRoad + EastTileIsRoad;

    if (Count == 4)
        return ERoadType::Cross;

    if (Count == 3)
    {
        if (!SouthTileIsRoad)
            return ERoadType::TShape_N;
        if (!NorthTileIsRoad)
            return ERoadType::TShape_S;
        if (!WestTileIsRoad)
            return ERoadType::TShape_E;
        if (!EastTileIsRoad)
            return ERoadType::TShape_W;
    }

    if (Count == 2)
    {
        if (NorthTileIsRoad && SouthTileIsRoad)
            return ERoadType::Straight_NS;
        if (EastTileIsRoad && WestTileIsRoad)
            return ERoadType::Straight_EW;

        if (NorthTileIsRoad && EastTileIsRoad)
            return ERoadType::Corner_NE;
        if (NorthTileIsRoad && WestTileIsRoad)
            return ERoadType::Corner_NW;
        if (SouthTileIsRoad && EastTileIsRoad)
            return ERoadType::Corner_SE;
        if (SouthTileIsRoad && WestTileIsRoad)
            return ERoadType::Corner_SW;
    }

    if (Count == 1)
    {
        if (NorthTileIsRoad)
            return ERoadType::Dead_N;
        if (SouthTileIsRoad)
            return ERoadType::Dead_S;
        if (EastTileIsRoad)
            return ERoadType::Dead_E;
        if (WestTileIsRoad)
            return ERoadType::Dead_W;
    }
    
    return ERoadType::Straight_NS;  // isolated tile -> fallback
}

UStaticMesh* ACityGeneratorII::GetRelevantMeshForRoad(ERoadType Type, float& OutYaw) const
{
    OutYaw = 0.f;

    switch (Type)
    {
        // if straights
        case ERoadType::Straight_EW:
            OutYaw = 0;
            return MeshRoadStraightNS;
        case ERoadType::Straight_NS:
            OutYaw = 90;
            return MeshRoadStraightNS;

        // if corners
        case ERoadType::Corner_NW:
            OutYaw = 0;
            return MeshRoadCornerNE;
        case ERoadType::Corner_SW:
            OutYaw = 90;
            return MeshRoadCornerNE;
        case ERoadType::Corner_SE:
            OutYaw = 180;
            return MeshRoadCornerNE;
        case ERoadType::Corner_NE:
            OutYaw = 270;
            return MeshRoadCornerNE;

        // if T-shapes
        case ERoadType::TShape_N:
            OutYaw = 0;
            return MeshRoadTShapeN;
        case ERoadType::TShape_W:
            OutYaw = 90;
            return MeshRoadTShapeN;
        case ERoadType::TShape_S:
            OutYaw = 180;
            return MeshRoadTShapeN;
        case ERoadType::TShape_E:
            OutYaw = 270;
            return MeshRoadTShapeN;

        // if cross
        case ERoadType::Cross:
            OutYaw = 0.f;
            return MeshRoadCross;

            // if dead ends
        case ERoadType::Dead_N:
            OutYaw = 0.f;
            return MeshRoadDeadEnd;
        case ERoadType::Dead_E:
            OutYaw = 90.f;
            return MeshRoadDeadEnd;
        case ERoadType::Dead_S:
            OutYaw = 180.f;
            return MeshRoadDeadEnd;
        case ERoadType::Dead_W:
            OutYaw = 270.f;
            return MeshRoadDeadEnd;

        // if my code sucks
        default:
            return nullptr;
    }
}
