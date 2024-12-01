#include "NoiseGenerator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"

uint8 UNoiseGenerator::NoiseToGray(float NoiseValue)
{
    float GrayValue = ((NoiseValue + 1) / 2) * 255;
    return FMath::RoundToInt(GrayValue);
}

float UNoiseGenerator::WhiteNoise()
{
    return FMath::FRandRange(-1.f, 1.f);
}

float UNoiseGenerator::PerlinNoise(FVector2D Location, TArray<int>& Permutation, int Octaves, float Frequency)
{
    float Total = 0.f;        // Final result after adding all octaves
    float MaxValue = 0.f;     // Used to normalize the result (ensures we get a value between -1 and 1)
    float Amplitude = 1.f;    // Amplitude for the current octave
    float CurrentFrequency = Frequency;

    for (int i = 0; i < Octaves; i++)
    {
        // Add the Perlin noise for the current octave
        Total += PerlinNoiseLayer(Location * CurrentFrequency, Permutation) * Amplitude;

        // Increase the frequency (the "zoom in" effect for each octave)
        MaxValue += Amplitude;

        // Reduce the amplitude for the next octave (this controls the "fade out" effect)
        Amplitude *= 0.5f;

        // Increase frequency for the next octave (higher frequency means smaller, more detailed features)
        CurrentFrequency *= 2.0f;
    }

    // Normalize the result to be in the range [-1, 1]
    return Total / MaxValue;
}

float UNoiseGenerator::VoronoiNoise(FVector2D Location, TArray<FVector2D>& Nuclei, int Octaves, float Frequency)
{
    float Total = 0.f;        // Accumulates results from all octaves
    float MaxValue = 0.f;     // Tracks the maximum possible amplitude
    float Amplitude = 1.f;    // Current amplitude
    float CurrentFrequency = Frequency; // Current frequency (starts at base)

    for (int i = 0; i < Octaves; i++)
    {
        // Scale location by the current frequency
        FVector2D ScaledLocation = Location * CurrentFrequency;

        // Get the Voronoi noise value for this layer
        float LayerValue = VoronoiNoiseLayer(ScaledLocation, Nuclei);

        // Add weighted contribution of this layer
        Total += LayerValue * Amplitude;

        // Accumulate the maximum amplitude
        MaxValue += Amplitude;

        // Prepare for the next octave
        Amplitude *= 0.5f;          // Halve the amplitude
        CurrentFrequency *= 2.0f;  // Double the frequency
    }

    // Normalize the result to [-1, 1]
    return Total / MaxValue;
}

float UNoiseGenerator::PerlinNoiseLayer(FVector2D Location, TArray<int>& Permutation)
{
    int X = FMath::FloorToInt(Location.X) & 255;
    int Y = FMath::FloorToInt(Location.Y) & 255;

    // Corner vectors
    FVector2D BottomLeft  = GetConstantVector(Permutation[(Permutation[X            ] + Y    ) % 256]);
    FVector2D BottomRight = GetConstantVector(Permutation[(Permutation[(X + 1) % 256] + Y    ) % 256]);
    FVector2D TopLeft     = GetConstantVector(Permutation[(Permutation[X            ] + Y + 1) % 256]);
    FVector2D TopRight    = GetConstantVector(Permutation[(Permutation[(X + 1) % 256] + Y + 1) % 256]);

    // Relative position in the cell
    float Xf = Location.X - FMath::FloorToInt(Location.X);
    float Yf = Location.Y - FMath::FloorToInt(Location.Y);

    // Vectors from corners to center
    FVector2D BottomLeftToCenter  = { Xf, Yf };
    FVector2D BottomRightToCenter = { Xf - 1.f, Yf };
    FVector2D TopLeftToCenter     = { Xf, Yf - 1.f };
    FVector2D TopRightToCenter    = { Xf - 1.f, Yf - 1.f };

    // Get dot products
    float DotBottomLeft  = BottomLeftToCenter.Dot(BottomLeft);
    float DotBottomRight = BottomRightToCenter.Dot(BottomRight);
    float DotTopLeft     = TopLeftToCenter.Dot(TopLeft);
    float DotTopRight    = TopRightToCenter.Dot(TopRight);

    float U = Smooth(Xf);
    float V = Smooth(Yf);

    float Interp1 = FMath::Lerp(DotBottomLeft,  DotTopLeft,  V);
    float Interp2 = FMath::Lerp(DotBottomRight, DotTopRight, V);

    float FinalResult = FMath::Lerp(Interp1, Interp2, U);

    return FinalResult;
}

float UNoiseGenerator::Smooth(float X)
{
    return X * X * X * (X * (X * 6.0f - 15.0f) + 10.0f);
}

FVector2D UNoiseGenerator::GetConstantVector(int Value)
{
    int h = Value & 3;

    if (h == 0)
        return { 1.f, 1.f };
    else if (h == 1)
        return { -1.f, 1.f };
    else if (h == 2)
        return { -1.f, -1.f };
    else
        return { 1.f, -1.f };
}

TArray<int> UNoiseGenerator::MakePermutation()
{
    TArray<int> Permutation;
    for (int i = 0; i < 256; i++)
    {
        Permutation.Add(i);
    }

    // Shuffle using Fisher-Yates algorithm
    for (int i = 255; i > 0; i--)
    {
        int RandNum = FMath::Rand() % (i + 1);
        Permutation.Swap(i, RandNum);
    }

    return Permutation;
}

float UNoiseGenerator::VoronoiNoiseLayer(FVector2D Location, TArray<FVector2D>& Nuclei)
{
    TArray<FVector2D> ClosestPoints = FindTwoClosest(Location, Nuclei);
    if (ClosestPoints.IsEmpty())
    {
        return -1.f;
    }

    float Dist1 = FMath::Sqrt(DistanceSquared(Location, ClosestPoints[0]));
    float Dist2 = FMath::Sqrt(DistanceSquared(Location, ClosestPoints[1]));

    float InterpValue = (Dist2 - Dist1) / (Dist2 + Dist1);

    return InterpValue * 2.f - 1.f;
}

float UNoiseGenerator::DistanceSquared(FVector2D A, FVector2D B)
{
    return FMath::Square(A.X - B.X) + FMath::Square(A.Y - B.Y);
}

TArray<FVector2D> UNoiseGenerator::MakeNuclei(int Width, int Height, float Frequency)
{
    TArray<FVector2D> Nuclei;

    int CellSize = 1 / Frequency;
    for (int Y = 0; Y < Frequency * Height; Y++)
    {
        for (int X = 0; X < Frequency * Width; X++)
        {
            int CellX = X * CellSize;
            int CellY = Y * CellSize;

            float NucleusX = FMath::FRandRange(float(CellX), CellX + CellSize);
            float NucleusY = FMath::FRandRange(float(CellY), CellY + CellSize);
            Nuclei.Add({ NucleusX, NucleusY });
        }
    }
    return Nuclei;
}

TArray<FVector2D> UNoiseGenerator::FindTwoClosest(FVector2D& Target, TArray<FVector2D>& Points)
{
    // Check if there are at least two points in the array
    if (Points.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("Insufficient points in the array."));
        return {};
    }

    // Initialize the two closest points and their distances
    FVector2D ClosestPoint1 = FVector2D::ZeroVector;
    FVector2D ClosestPoint2 = FVector2D::ZeroVector;
    float ClosestDist1 = FLT_MAX;
    float ClosestDist2 = FLT_MAX;

    // Iterate through all points in the array
    for (const FVector2D& Point : Points)
    {
        // Calculate the squared distance (faster than sqrt for comparison)
        float DistanceSq = FVector2D::DistSquared(Target, Point);

        // Check if this point is closer than the first closest
        if (DistanceSq < ClosestDist1)
        {
            // Shift the first closest to the second closest
            ClosestDist2 = ClosestDist1;
            ClosestPoint2 = ClosestPoint1;

            // Update the first closest
            ClosestDist1 = DistanceSq;
            ClosestPoint1 = Point;
        }
        else if (DistanceSq < ClosestDist2)
        {
            // Update the second closest only
            ClosestDist2 = DistanceSq;
            ClosestPoint2 = Point;
        }
    }

    // Return the two closest points
    return { ClosestPoint1, ClosestPoint2 };
}

UTexture2D* UNoiseGenerator::CreateNoise(FName FolderPath, FName AssetName, ENoiseType NoiseType, int Width, int Height, int Seed, int Octaves, float Frequency)
{
    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid texture dimensions: Width=%d, Height=%d"), Width, Height);
        return nullptr;
    }

    if (Frequency == 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid frequency : cannot be 0"));
        return nullptr;
    }

    if (FolderPath.IsNone() || AssetName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid FolderPath or AssetName."));
        return nullptr;
    }

    // Construct the full path for the new asset (e.g., /Game/MyTextures/MyTexture)
    FString FullPath = FolderPath.ToString() + "/" + AssetName.ToString();
    if (!FPackageName::IsValidLongPackageName(FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid package path: %s"), *FullPath);
        return nullptr;
    }

    // Create or find the package where the asset will be stored
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package at : %s"), *FullPath);
        return nullptr;
    }

    // Create a new Texture2D object inside the package
    UTexture2D* NewTexture = NewObject<UTexture2D>(Package, AssetName, RF_Public | RF_Standalone);
    if (!NewTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UTexture2D object."));
        return nullptr;
    }

    // Initialize the texture's source data (dimensions, number of mipmaps, format)
    NewTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8);

    // Lock the first mipmap to write pixel data
    uint8* MipData = NewTexture->Source.LockMip(0);
    if (!MipData)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to lock texture mip data."));
        return nullptr;
    }

    //Seeds the randomness
    FMath::RandInit(Seed);

    switch (NoiseType)
    {
    case ENoiseType::WhiteNoise:
    {
        // Write white noise pixel data
        for (int Y = 0; Y < Height; ++Y)
        {
            for (int X = 0; X < Width; ++X)
            {
                // Calculate pixel index (4 bytes per pixel in BGRA8 format)
                int PixelIndex = ((Y * Width) + X) * 4;

                // Generate a grayscale value
                float NoiseValue = WhiteNoise();
                uint8 GrayValue = NoiseToGray(NoiseValue);
                MipData[PixelIndex + 0] = GrayValue; // Blue channel
                MipData[PixelIndex + 1] = GrayValue; // Green channel
                MipData[PixelIndex + 2] = GrayValue; // Red channel
                MipData[PixelIndex + 3] = 255;       // Alpha channel
            }
        }
        break;
    }

    case ENoiseType::PerlinNoise:
    {
        //Make a permuation array
        TArray<int> PermuationArray = MakePermutation();

        // Write white noise pixel data
        for (int Y = 0; Y < Height; Y++)
        {
            for (int X = 0; X < Width; X++)
            {
                // Calculate pixel index (4 bytes per pixel in BGRA8 format)
                int PixelIndex = ((Y * Width) + X) * 4;

                // Generate a grayscale value
                float NoiseValue = PerlinNoise({ float(X), float(Y) }, PermuationArray, Octaves, Frequency);
                uint8 GrayValue = NoiseToGray(NoiseValue);
                MipData[PixelIndex + 0] = GrayValue; // Blue channel
                MipData[PixelIndex + 1] = GrayValue; // Green channel
                MipData[PixelIndex + 2] = GrayValue; // Red channel
                MipData[PixelIndex + 3] = 255;       // Alpha channel
            }
        }
        break;
    }

    case ENoiseType::VoronoiNoise:

        // The nucleus is the central part of the cell
        TArray<FVector2D> Nuclei = MakeNuclei(Width, Height, Frequency);

        // Write white noise pixel data
        for (int Y = 0; Y < Height; ++Y)
        {
            for (int X = 0; X < Width; ++X)
            {
                // Calculate pixel index (4 bytes per pixel in BGRA8 format)
                int PixelIndex = ((Y * Width) + X) * 4;

                // Generate a grayscale value
                float NoiseValue = VoronoiNoise(FVector2D(X, Y), Nuclei, Octaves, Frequency);
                uint8 GrayValue = NoiseToGray(NoiseValue);
                MipData[PixelIndex + 0] = GrayValue; // Blue channel
                MipData[PixelIndex + 1] = GrayValue; // Green channel
                MipData[PixelIndex + 2] = GrayValue; // Red channel
                MipData[PixelIndex + 3] = 255;       // Alpha channel
            }
        }
        break;
    }

    // Unlock the mipmap to commit changes
    NewTexture->Source.UnlockMip(0);

    // Update texture properties
    NewTexture->SRGB = true; // Use sRGB color space for textures viewed directly on screen
    NewTexture->CompressionSettings = TC_Grayscale; // Use default compression for regular textures
    NewTexture->PostEditChange(); // Apply changes to the asset

    // Notify the asset registry that a new asset has been created
    FAssetRegistryModule::AssetCreated(NewTexture);

    // Mark the package as modified so it can be saved
    Package->MarkPackageDirty();

    // Save the package to disk
    FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
    if (UPackage::SavePackage(Package, NewTexture, RF_Public | RF_Standalone, *PackageFileName))
    {
        UE_LOG(LogTemp, Log, TEXT("Texture2D asset created successfully at : %s"), *FullPath);
        return NewTexture;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save Texture2D asset at : %s"), *FullPath);
        return nullptr;
    }
}
