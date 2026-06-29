// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGeneratorII.h"

#include "Components/InstancedStaticMeshComponent.h"

DEFINE_LOG_CATEGORY(LogCityBuilder)

#pragma region Common

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

#pragma endregion

#pragma region Generation Pipeline

void ACityGeneratorII::Generate()
{
    Clear();
    GridSize = FMath::Max(GridSize, 5);

    InitGrid();

    Rand = FRandomStream(Seed);
    PlaceRoads();
    PlaceBuildings();
    SpawnInstances();
}

void ACityGeneratorII::Clear()
{
    ClearISMCs();
    Grid.Empty();
}

void ACityGeneratorII::InitGrid()
{
    const int Total = GridSize * GridSize;
    Grid.Init(ETileType::Empty, Total);
    BuildingID.Init(-1, Total);
}

void ACityGeneratorII::PlaceRoads()
{
    int center = GridSize / 2;

#pragma region Outer perimeter
    RoadHorizontalLine(0, GridSize - 1, 0);
    RoadHorizontalLine(0, GridSize - 2, 0);
    RoadHorizontalLine(0, GridSize - 1, GridSize - 1);
    RoadHorizontalLine(0, GridSize - 2, GridSize - 2);
    RoadVerticalLine(0, 0, GridSize - 1);
    RoadVerticalLine(0, 0, GridSize - 2);
    RoadVerticalLine(GridSize - 1, 0, GridSize - 1);
    RoadVerticalLine(GridSize - 2, 0, GridSize - 2);
#pragma endregion


#pragma region Quadrants cross
    bool oddWidth = MainRoadWidth % 2 == 1;
    const int halfMainRoadWidth = MainRoadWidth / 2;
    for (int offset = -halfMainRoadWidth; offset < halfMainRoadWidth + oddWidth; offset++)
    {
        RoadHorizontalLine(0, GridSize - 1, center + offset);
        RoadVerticalLine(center + offset, 0, GridSize - 1);
    }
#pragma endregion

#pragma region Landmark dedicated space
    /*const int halfLandmarkDedicatedSpaceSize = LandmarkDedicatedSpaceSize / 2;
    const int centerMinusHalf = center - halfLandmarkDedicatedSpaceSize;
    const int centerPlusHalf = center + halfLandmarkDedicatedSpaceSize;
    for (int offset = centerMinusHalf; offset < centerPlusHalf; offset++)
        RoadHorizontalLine(
            centerMinusHalf,
            centerPlusHalf - 1,
            offset
        );*/
#pragma endregion

    // includes borders
    const TArray<FQuadrant> quadrants = 
    {
        { 1,        1,        center - 1, center - 1 },  // topleft
        { center + 1, 1,        GridSize - 2,   center - 1 },  // topright
        { 1,        center + 1, center - 1, GridSize - 2   },  // bottomleft
        { center + 1, center + 1, GridSize - 2,   GridSize - 2   },  // bottomright
    };


    for (const FQuadrant& quad : quadrants)
    {
        EFillMethod fillMethod = (EFillMethod) Rand.RandRange(0, (int) EFillMethod::Length - 1);

        // auto [XO, Y0, X1, Y1] = quad;
        switch (fillMethod)
        {
        case EFillMethod::Subdivison:
            SubdivideBlock(quad.X0, quad.Y0, quad.X1, quad.Y1, 0);
            break;
            
        case EFillMethod::PlainGrid:
            FillPlainGrid(quad.X0, quad.Y0, quad.X1, quad.Y1);
            break;

        case EFillMethod::Length: // i messed up somewhere
        default:
            UE_LOG(LogCityBuilder, Error, TEXT("Messed up"));
            break; // i messed up somewhere

        }

    }

    //// Recursively subdivide the interior
    // SubdivideBlock(1, 1, GridSize - 2, GridSize - 2, 0); // do whole map
    
}

void ACityGeneratorII::PlaceBuildings()
{
    int index;
    for (int Y = 0; Y < GridSize; ++Y)
    for (int X = 0; X < GridSize; ++X)
        if (Grid[index = Index(X, Y)] == ETileType::Empty)
            Grid[index] = ETileType::Building;
}

#pragma endregion

#pragma region Fill methods

//void ACityGeneratorII::SubdivideBlock(int X0, int Y0, int X1, int Y1, int Depth)
//{
//    int Width = X1 - X0;
//    int Height = Y1 - Y0;
//
//    // stop subdividing if block too small
//    if (Width < MinBlockSize && Height < MinBlockSize)
//        return;
//
//    // Bias: prefer splitting the longer axis, but sometimes flip randomly
//    bool bSplitHorizontal = (Height > Width);
//    if (Rand.FRand() > .5f && Depth < MaxDepth)
//        bSplitHorizontal = !bSplitHorizontal;
//
//    if (bSplitHorizontal && Height >= MinBlockSize)
//    {
//        // Pick a split Y somewhere in the middle third (avoids tiny slivers)
//        int Margin = FMath::Max(1, Height / 4);
//        int SplitY = Rand.RandRange(Y0 + Margin, Y1 - Margin);
//
//        for (int X = X0; X <= X1; ++X)
//            SetRoad(X, SplitY);
//
//        SubdivideBlock(X0, Y0, X1, SplitY - 1, Depth + 1);
//        SubdivideBlock(X0, SplitY + 1, X1, Y1, Depth + 1);
//    }
//    else if (!bSplitHorizontal && Width >= MinBlockSize)
//    {
//        int Margin = FMath::Max(1, Width / 4);
//        int SplitX = Rand.RandRange(X0 + Margin, X1 - Margin);
//
//        for (int Y = Y0; Y <= Y1; ++Y)
//            SetRoad(SplitX, Y);
//
//        SubdivideBlock(X0, Y0, SplitX - 1, Y1, Depth + 1);
//        SubdivideBlock(SplitX + 1, Y0, X1, Y1, Depth + 1);
//    }
//}


void ACityGeneratorII::SubdivideBlock(int X0, int Y0, int X1, int Y1, int Depth)
{
    int Width = X1 - X0;
    int Height = Y1 - Y0;

    if (Width < MinBlockSize && Height < MinBlockSize)
        return;

    const int Half = RoadWidth / 2;

    bool bSplitHorizontal = (Height > Width);
    if (Rand.FRand() > .5f && Depth < MaxDepth)
        bSplitHorizontal = !bSplitHorizontal;

    if (bSplitHorizontal && Height >= MinBlockSize)
    {
        int Margin = FMath::Max(RoadWidth, Height / 4);

        // Bail out if there isn't enough room for two blocks + one road band
        if (Y0 + Margin > Y1 - Margin)
            return;

        int SplitY = Rand.RandRange(Y0 + Margin, Y1 - Margin);

        // Paint every lane of the road band
        for (int Lane = 0; Lane < RoadWidth; ++Lane)
            for (int X = X0; X <= X1; ++X)
                SetRoad(X, SplitY - Half + Lane);

        // Child blocks start/end outside the full road band
        SubdivideBlock(X0, Y0, X1, SplitY - Half - 1, Depth + 1);
        SubdivideBlock(X0, SplitY + Half, X1, Y1, Depth + 1);
    }
    else if (!bSplitHorizontal && Width >= MinBlockSize)
    {
        int Margin = FMath::Max(RoadWidth, Width / 4);

        if (X0 + Margin > X1 - Margin)
            return;

        int SplitX = Rand.RandRange(X0 + Margin, X1 - Margin);

        for (int Lane = 0; Lane < RoadWidth; ++Lane)
            for (int Y = Y0; Y <= Y1; ++Y)
                SetRoad(SplitX - Half + Lane, Y);

        SubdivideBlock(X0, Y0, SplitX - Half - 1, Y1, Depth + 1);
        SubdivideBlock(SplitX + Half, Y0, X1, Y1, Depth + 1);
    }
}

//void ACityGeneratorII::FillPlainGrid(int X0, int Y0, int X1, int Y1)
//{
//    for (; Y0 < Y1; ++Y0)
//    for (; X0 < X1; ++X0)
//        if (X0 % BlockSize == 0 || Y0 % BlockSize == 0)
//            SetRoad(X0, Y0); // road when either X or Y is on a block boundary
//}

void ACityGeneratorII::FillPlainGrid(int X0, int Y0, int X1, int Y1)
{
    for (int Y = Y0; Y < Y1; Y += BlockSize)
    for (int road = 0; road < RoadWidth && Y + road < Y1; road++)
        RoadHorizontalLine(X0, X1, Y + road);
 
    for (int X = X0; X < X1; X += BlockSize)
    for (int road = 0; road < RoadWidth && X + road < X1; road++)
        RoadVerticalLine(X + road, Y0, Y1);
}

#pragma endregion

#pragma region Instanced meshes handling

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

                GetOrCreateISMC(Mesh)->AddInstance(MakeTransformForTile(X, Y, Yaw));
            }
            else if (Type == ETileType::Building)
            {
                TArray<UStaticMesh*> buildingMeshes = <:this:>(EQuadrant quadrant)
                <%
                    switch (quadrant)
                    {
                    case EQuadrant::TopLeft:  return BuildingMeshesQuad1;
                    case EQuadrant::TopRight: return BuildingMeshesQuad2;
                    case EQuadrant::BottomLeft: return BuildingMeshesQuad3;
                    case EQuadrant::BottomRight: return BuildingMeshesQuad4;

                    case EQuadrant::None:
                    default: return TArray<UStaticMesh*>{};
                    }
                %> (GetTileQuadrant(X, Y));

                if (buildingMeshes.Num() == 0)
                    continue;
                
                UStaticMesh* Mesh = buildingMeshes[Rand.RandRange(0, buildingMeshes.Num() - 1)];
                if (!Mesh)
                    continue;

                const FTransform& transform = MakeTransformForTile(X, Y, 0.f);
                GetOrCreateISMC(Mesh)->AddInstance(transform);
                GetOrCreateISMC(MeshRoadCross)->AddInstance(transform);
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

#pragma endregion

#pragma region Road

ERoadType ACityGeneratorII::GetRelevantRoadTile(int X, int Y) const
{
    const bool NorthTileIsRoad = IsRoad(X, Y + 1);
    const bool SouthTileIsRoad = IsRoad(X, Y - 1);
    const bool EastTileIsRoad = IsRoad(X + 1, Y);
    const bool WestTileIsRoad = IsRoad(X - 1, Y);
    const int Count = NorthTileIsRoad + SouthTileIsRoad + WestTileIsRoad + EastTileIsRoad;


    if (Count == 4)
    {
       return ERoadType::Cross;
       /* const bool HasAnyDiagonalRoad =
            IsRoad(X + 1, Y + 1) ||
            IsRoad(X - 1, Y + 1) ||
            IsRoad(X + 1, Y - 1) ||
            IsRoad(X - 1, Y - 1)
        ;

        return HasAnyDiagonalRoad ? ERoadType::Plain : ERoadType::Cross;*/
    }

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
        case ERoadType::Straight_EW:
            OutYaw = 0;
            return MeshRoadStraightNS;
        case ERoadType::Straight_NS:
            OutYaw = 90;
            return MeshRoadStraightNS;

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

        case ERoadType::Cross:
            OutYaw = 0.f;
            return MeshRoadCross;
        case ERoadType::Plain:
            OutYaw = 0.f;
            return MeshRoadPlain;

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

#pragma endregion



void ACityGeneratorII::SpawnRandomObject()
{
    if (objectsToSpawn.Num() == 0)
        return;

    int x;
    int y;
    do
    {
        x = Rand.RandRange(0, GridSize);
        y = Rand.RandRange(0, GridSize);
    } while (!IsRoad(x, y));
    GetWorld()->SpawnActor<AActor>(
        objectsToSpawn[FMath::RandRange(0, objectsToSpawn.Num() - 1)],
        FTransform(
            FRotator::ZeroRotator, 
            FVector(
                (x + 0.5f) * TileSize * Scale,
                (y + 0.5f) * TileSize * Scale,
                0.f
            ) + GetActorLocation(),
            FVector::OneVector
        ),
        spawnParams
    );
}
