#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "NoiseGenerator.generated.h"

UENUM(BlueprintType)
enum class ENoiseType : uint8
{
	WhiteNoise   UMETA(DisplayName = "White Noise"),
	PerlinNoise  UMETA(DisplayName = "Perlin Noise"),
	VoronoiNoise UMETA(DisplayName = "Voronoi Noise")
};

UCLASS()
class TP2_API UNoiseGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:

	// Converts a noise value in the range [-1, 1] to a grayscale value [0, 255]
	static uint8 NoiseToGray(float NoiseValue);

	// Generates a random noise value between -1 and 1
	static float WhiteNoise();
	// Computes Perlin noise with support for octaves and frequency scaling
	static float PerlinNoise(FVector2D Location, TArray<int>& Permutation, int Octaves, float Frequency);
	// Computes Voronoi noise, which generates a cellular-like texture
	static float VoronoiNoise(FVector2D Location, TArray<FVector2D>& Nuclei, int Octaves, float Frequency);

	// Perlinspecific methods
	// Generates a layer of Perlin noise for a specific octave
	static float PerlinNoiseLayer(FVector2D Location, TArray<int>& Permutation);
	// Smooths a value using a quintic polynomial for easing
	static float Smooth(float X);
	// Maps an integer to one of four constant directional vectors for gradient noise
	static FVector2D GetConstantVector(int Value);
	// Shuffles an array using Fisher-Yates algorithm to create a random permutation
	static TArray<int> MakePermutation();

	// Voronoi specific methods
	// Computes a single layer of Voronoi noise based on distances to nuclei
	static float VoronoiNoiseLayer(FVector2D Location, TArray<FVector2D>& Nuclei);
	static float DistanceSquared(FVector2D A, FVector2D B);
	// Creates nuclei points for Voronoi noise generation based on frequency
	static TArray<FVector2D> MakeNuclei(int Width, int Height, float Frequency);
	// Finds the two closest points to a target point in a given array
	static TArray<FVector2D> FindTwoClosest(FVector2D& Target, TArray<FVector2D>& Points);
	
public:

	// Creates a procedural noise texture and saves it as an asset in the Unreal Content Browser
	UFUNCTION(BlueprintCallable)
	static UTexture2D* CreateNoise(FName FolderPath = "/Game", FName AssetName = "T_Noise", ENoiseType NoiseType = ENoiseType::WhiteNoise, int Width = 256, int Height = 256, int Seed = 0, int Octaves = 1, float Frequency = 0.05f);
	
};
