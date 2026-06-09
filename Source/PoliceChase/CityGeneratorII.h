// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "CityGeneratorII.generated.h"


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
    Dead_N UMETA(DisplayName = "Dead End N"),
    Dead_S UMETA(DisplayName = "Dead End S"),
    Dead_E UMETA(DisplayName = "Dead End E"),
    Dead_W UMETA(DisplayName = "Dead End W"),
};


UCLASS(BlueprintType, Blueprintable)
class ACityGeneratorII : public AActor
{
    GENERATED_BODY()

public:
    ACityGeneratorII();

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Generation")
    int Seed = 42;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Generation")
    float TileSize = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "5"), Category = "City|Generation")
    int GridSize = 12;

    // purely grid based city (arranged in blocks)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "2"), Category = "City|Generation")
    int BlockSize = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "2"), Category = "City|Generation")
    int MinBlockSize = 4;   // smallest block before stopping
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), Category = "City|Generation")
    int MaxDepth = 6;   // cap recursion to avoid micro-streets everywhere

    // likelihhod of merging two buildings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"), Category = "City|Generation")
    float MergeProbability = .0f/*.35f*/;

    // 1x1 buildings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    TArray<UStaticMesh*> BuildingMeshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadStraightNS;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadCornerNE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadTShapeN;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadCross;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Meshes")
    UStaticMesh* MeshRoadDeadEnd;

    
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "City")
    void Generate();

    /** Destroy all spawned instances and clear state. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "City")
    void Clear();

protected:
    virtual void BeginPlay() override;

private:
    
    TArray<ETileType> Grid;
    TArray<int> BuildingID;  // which mesh index (-1 = none)
    TArray<bool> MergedEast;  // tile is the LEFT half of a 2×1

    UPROPERTY()
    TMap<UStaticMesh*, UInstancedStaticMeshComponent*> ISMCMap;

    void InitGrid();
    void PlaceRoads();
    void SubdivideBlock(int X0, int Y0, int X1, int Y1, int Depth);
    void PlaceBuildings(FRandomStream& Rand);
    void SpawnInstances();
    void ClearISMCs();

    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh);

    inline int Index(int X, int Y) const { return Y * GridSize + X; }
    inline void SetRoad(int X, int Y)
    {
        if (X < 0 || X >= GridSize)
            return;
        if (Y < 0 || Y >= GridSize)
            return;

        Grid[Index(X, Y)] = ETileType::Road;
    }
    inline bool  IsRoad(int X, int Y) const;

    ERoadType GetRelevantRoadTile(int X, int Y) const;
    FTransform MakeTransformForTile(int X, int Y, float Yaw = 0.f, float ScaleX = 1.f) const;

    UStaticMesh* GetRelevantMeshForRoad(ERoadType Type, float& OutYaw) const;
};
