// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "CityGeneratorII.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCityBuilder, Log, All)

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Empty UMETA(DisplayName = "Empty"),
    Road UMETA(DisplayName = "Road"),
    Building UMETA(DisplayName = "Building"),
};


UENUM(BlueprintType)
enum class ERoadType : uint8
{
    Straight_NS UMETA(DisplayName = "Straight N-S"),
    Straight_EW UMETA(DisplayName = "Straight E-W"),
    Corner_NE UMETA(DisplayName = "Corner N-E"),
    Corner_NW UMETA(DisplayName = "Corner N-W"),
    Corner_SE UMETA(DisplayName = "Corner S-E"),
    Corner_SW UMETA(DisplayName = "Corner S-W"),
    TShape_N UMETA(DisplayName = "TShape North"),
    TShape_S UMETA(DisplayName = "TShape South"),
    TShape_E UMETA(DisplayName = "TShape East"),
    TShape_W UMETA(DisplayName = "TShape West"),
    Cross UMETA(DisplayName = "Cross"),
    Plain UMETA(DisplayName = "Plain"),
    Dead_N UMETA(DisplayName = "Dead End N"),
    Dead_S UMETA(DisplayName = "Dead End S"),
    Dead_E UMETA(DisplayName = "Dead End E"),
    Dead_W UMETA(DisplayName = "Dead End W"),
};

UENUM(BlueprintType)
enum class EQuadrant : uint8
{
    TopLeft UMETA(DisplayName = "Top Left"),
    TopRight UMETA(DisplayName = "Top Right"),
    BottomLeft UMETA(DisplayName = "Bottom Left"),
    BottomRight UMETA(DisplayName = "Bottom Right"),
    None UMETA(DisplayName = "None"),  // cross / landmark / perimeter
};

struct FQuadrant
{
    int X0;
    int Y0;
    int X1;
    int Y1;
};


enum class EFillMethod: uint8
{
    Subdivison,
    PlainGrid,
    Length,
};

UCLASS(BlueprintType, Blueprintable)
class ACityGeneratorII : public AActor
{
    GENERATED_BODY()

public:
    ACityGeneratorII();

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Generation")
    int Seed = 42;
private:
    FRandomStream Rand;
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Generation")
    float Scale = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Generation")
    float TileSize = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "5"), Category = "City|Generation")
    int GridSize = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "City|Generation")
    int RoadWidth = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "City|Generation")
    int MainRoadWidth = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "City|Generation")
    int LandmarkDedicatedSpaceSize = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "2"), Category = "City|Generation|GridFill")
    int BlockSize = 3; // my code musty be ass cuz it acts as blockSize - RoadWidth so account for that

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "City|Generation|SubdivideFill")
    int MinBlockSize = 4;   // smallest block before stopping
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), Category = "City|Generation|SubdivideFill")
    int MaxDepth = 6;   // cap recursion to avoid micro-streets everywhere

#pragma region Meshes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshesQuad1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshesQuad2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshesQuad3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshesQuad4;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadStraightNS;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadCornerNE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadTShapeN;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadCross;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadPlain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadDeadEnd;
#pragma endregion
    
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "City")
    void Generate();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "City")
    void Clear();

protected:
    virtual void BeginPlay() override;

private:
    
    TArray<ETileType> Grid;
    TArray<int> BuildingID;  // which mesh index (-1 -> none)

    UPROPERTY()
    TMap<UStaticMesh*, UInstancedStaticMeshComponent*> ISMCMap;

#pragma region Generation Pipeline
    void InitGrid();
    void PlaceRoads();
    void PlaceBuildings();
#pragma endregion


#pragma region Fill methods
    void SubdivideBlock(int X0, int Y0, int X1, int Y1, int Depth);
    void FillPlainGrid(int X0, int Y0, int X1, int Y1);
#pragma endregion


#pragma region Instanced mesh handling
    void SpawnInstances();
    void ClearISMCs();

    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh);
#pragma endregion


    inline int Index(int X, int Y) const { return Y * GridSize + X; }

#pragma region Road Helpers
    inline void SetRoad(int X, int Y)
    {
        if (X < 0 || X >= GridSize)
            return;
        if (Y < 0 || Y >= GridSize)
            return;

        Grid[Index(X, Y)] = ETileType::Road;
    }

    inline bool IsRoad(int X, int Y) const
    {
        if (X < 0 || X >= GridSize)
            return false;
        if (Y < 0 || Y >= GridSize)
            return false;

        return Grid[Index(X, Y)] == ETileType::Road;
    }

    inline void SetLargeRoad(int X, int Y)
    {
        SetRoad(X, Y);

        /*const int Half = RoadWidth / 2;
        for (int DY = 0; DY < RoadWidth; ++DY)
            for (int DX = 0; DX < RoadWidth; ++DX)
                SetRoad(X - Half + DX, Y - Half + DY);*/
    }

    inline void RoadHorizontalLine(int X0, int X1, int Y)
    {
        for (int X = X0; X <= X1; ++X)
            SetLargeRoad(X, Y);
    }

    inline void RoadVerticalLine(int X, int Y0, int Y1) 
    {
        for (int Y = Y0; Y <= Y1; ++Y)
            SetLargeRoad(X, Y);
    }
#pragma endregion

    ERoadType GetRelevantRoadTile(int X, int Y) const;
    inline FTransform MakeTransformForTile(int X, int Y, float Yaw = 0.f) const
    {
        return FTransform(
            FRotator(0.f, Yaw, 0.f),
            FVector(
                (X + 0.5f) * TileSize * Scale,
                (Y + 0.5f) * TileSize * Scale,
                0.f
            ) + GetActorLocation(),
            FVector::OneVector * Scale
        );
    }

    UStaticMesh* GetRelevantMeshForRoad(ERoadType Type, float& OutYaw) const;

#pragma region Quadrants helper
    inline EQuadrant GetTileQuadrant(int X, int Y) const
    {
        int center = GridSize / 2;
        // Perimeter
        if (X == 0 || X == GridSize - 1 || Y == 0 || Y == GridSize - 1)
            return EQuadrant::None;

        // Central cross band
        if (X == center || Y == center)
            return EQuadrant::None;

        if (X < center && Y < center)
            return EQuadrant::TopLeft;
        if (X > center && Y < center)
            return EQuadrant::TopRight;
        if (X < center && Y > center)
            return EQuadrant::BottomLeft;
        return EQuadrant::BottomRight;
    }

    inline bool IsInQuadrant(int X, int Y, EQuadrant Quadrant) const
    {
        return GetTileQuadrant(X, Y) == Quadrant;
    }
#pragma endregion


    public:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
        TArray<TSubclassOf<AActor>> objectsToSpawn;
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
        float OffsetToGround = 100.f;

        UFUNCTION(CallInEditor, BlueprintCallable, Category = "Spawn")
        void SpawnRandomObject();
    private:
        const FActorSpawnParameters spawnParams = <:=:>()
        <%
            FActorSpawnParameters spawnParams_;
            spawnParams_.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            spawnParams_.TransformScaleMethod = ESpawnActorScaleMethod::SelectDefaultAtRuntime;
            return spawnParams_;
        %>();
       
};
