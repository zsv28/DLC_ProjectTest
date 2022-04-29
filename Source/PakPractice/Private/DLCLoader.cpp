// Fill out your copyright notice in the Description page of Project Settings.


#include "DLCLoader.h"
#include "CoreUObject/Public/Misc/PackageName.h"
#include "Engine/Classes/Engine/AssetManager.h"
#include "Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "IPlatformFilePak.h"
#include "MapInfoAsset.h"
#include <Containers/UnrealString.h>

// DLC loading
// 1. Search dlc *.pak files in "<GameFolder>/DLC/" path
// 2. *.pak's name must be equivalently to plugin name
// 3. Use PakLoader plugin for load and mount plugins
//
// Retun: array of loaded plugin's names


bool UDLCLoader::ReadDLCMapsInfo()
{
#if UE_BUILD_SHIPPING
//
#else
	const UAssetManager * assetManager = &UAssetManager::Get();
	TArray<FString> classesToLoad = GetClassesToLoad();
	TArray<FAssetData> RegisteryData;
	if (assetManager->GetAssetRegistry().GetAllAssets(RegisteryData))
	{
		for (FAssetData &data : RegisteryData)
		{
			for (FString classToLoad : classesToLoad)
			{
				if (data.AssetName == FName(*classToLoad))
				{
					UObject * objectToLoad = nullptr;
					auto generatedClassPath = data.TagsAndValues.FindTag(FName("GeneratedClass"));
					if (generatedClassPath.IsSet())
					{
						const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*generatedClassPath.GetValue());
						const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);
						objectToLoad = FStringAssetReference(ClassObjectPath).TryLoad();
					}
					if (objectToLoad)
					{
						const UBlueprintGeneratedClass* generatedClass = Cast<UBlueprintGeneratedClass>(objectToLoad);
						const UMapInfoAsset * mapInfoObject = Cast<UMapInfoAsset>(generatedClass->GetDefaultObject());
						if (mapInfoObject)
						{
							_mapsInfo.Add(mapInfoObject->_mapInfo);
						}
					}
				}
			}
		}
	}
#endif
	LoadDLC();
	return _mapsInfo.Num() > 0;
}

TArray<FMapInfo> UDLCLoader::GetMapsInfo()
{
	if(_mapsInfo.Num() == 0)
	{
		ReadDLCMapsInfo();
	}
	return _mapsInfo;
}

void UDLCLoader::Clear()
{
	_mapsInfo.Empty();
}

TArray<FString> UDLCLoader::GetClassesToLoad()
{
	TArray<FString> outClasses; 
   // Chapter2/BP_Chapter2Map1InfoAsset.BP_Chapter2Map1InfoAsset
    TArray<FName> baseClasses;
    TSet<FName> exludedClasses;
    TSet<FName> derivedClasses;
    baseClasses.Add(UMapInfoAsset::StaticClass()->GetFName());
    UAssetManager* assetManager = &UAssetManager::Get();
    assetManager->GetAssetRegistry().GetDerivedClassNames(baseClasses, exludedClasses, derivedClasses);
    for (FName derived : derivedClasses)
    {
	    /*Trim class name to get ClasName instead of ClassName.ClassName_C*/
	    FString derivedString = derived.ToString();
	    if (derivedString.EndsWith("_C"))
			derivedString = derivedString.Mid(0, derivedString.Len() - 2);
    	
	    outClasses.Add(*derivedString);
    }
    return outClasses;

}

TArray<FString> UDLCLoader::LoadDLC()
{
	TArray<FString> LoadedPlugins;
#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("Run in Editor mode. DLC loadin gaborted"));
	return LoadedPlugins;
#endif
	// Get DLC folder
	FString PathToDLCFolder = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) + "DLC";
	UE_LOG(LogTemp, Warning, TEXT(" PATH: %s "), *PathToDLCFolder);
	IFileManager& FileManager = IFileManager::Get();
	if (!FPaths::DirectoryExists(PathToDLCFolder))
	{
		UE_LOG(LogTemp, Warning, TEXT("DLC Directory %s not found"),*PathToDLCFolder);
		FileManager.MakeDirectory(*PathToDLCFolder, true);
		return LoadedPlugins;
	}
	// Get all *.pak files and try to load plugins
	TArray<FString> PAKFiles;
	FString DLCExtension = "*.pak";
	FileManager.FindFilesRecursive(PAKFiles, *PathToDLCFolder,*DLCExtension, true, false);
	for (size_t i = 0; i < PAKFiles.Num(); i++)
	{
		MountDLC(PAKFiles[i]);ReadPakFile(PAKFiles[i]);
	}
	return LoadedPlugins;
}

bool UDLCLoader::MountDLC(const FString& PakFilename)
{
	int32 PakOrder = GetDLCOrder(PakFilename);
	FPakPlatformFile* dlcFile = GetDLCFile();
	TArray<FString> mountedPoints1;
	dlcFile->GetMountedPakFilenames(mountedPoints1);
	bool result = dlcFile->Mount(*PakFilename, PakOrder, NULL);
	TArray<FString> mountedPoints2;
	dlcFile->GetMountedPakFilenames(mountedPoints2);
	return result;
}

bool UDLCLoader::UnmountDLC(const FString& PakFilename)
{
	int32 PakOrder = GetDLCOrder(PakFilename);
	FPakPlatformFile* dlcFile = GetDLCFile();
	TArray<FString> mountedPoints1;
	dlcFile->GetMountedPakFilenames(mountedPoints1);
	if(mountedPoints1.Contains(PakFilename))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mount point %s exists! Unmountstarted..."), *PakFilename);
		if(dlcFile->Unmount(*PakFilename))
		{
		UE_LOG(LogTemp, Warning, TEXT("Unmounted!"));
		}
		else
		{
		UE_LOG(LogTemp, Error, TEXT("Unmounting error!"));
		return false;
		}
	}
	return true;
}

int32 UDLCLoader::GetDLCOrder(const FString& PakFilePath)
{
	if (PakFilePath.StartsWith(FString::Printf(TEXT("%sPaks/%s-"),*FPaths::ProjectContentDir(), FApp::GetProjectName())))
    {
		return 4;
    }
    else if (PakFilePath.StartsWith(FPaths::ProjectContentDir()))
    {
		return 3;
    }
    else if (PakFilePath.StartsWith(FPaths::EngineContentDir()))
    {
		return 2;
    }
    else if (PakFilePath.StartsWith(FPaths::ProjectSavedDir()))
    {
		return 1;
    }
	
	return 0;

}

void UDLCLoader::RegisterMountPoint(const FString& RootPath, const FString& ContentPath)
{
	FPackageName::RegisterMountPoint(RootPath, ContentPath);
}

TArray<FString> UDLCLoader::GetFilesInDLC(const FString& Directory)
{
	FDLCLoaderFileVisitor Visitor;
	GetDLCFile()->IterateDirectory(*Directory, Visitor);
	return Visitor.Files;
}

bool UDLCLoader::ReadPakFile(FString PakFileName)
{
	UE_LOG(LogTemp, Warning, TEXT("ReadPakFile: Mount pak file : %s"),*PakFileName);
	FPakPlatformFile* PakPlatformFile;
	{
		FString PlatformFileName = FPlatformFileManager::Get().GetPlatformFile().GetName();
		if (PlatformFileName.Equals(FString(TEXT("PakFile"))))
		{
			PakPlatformFile =static_cast<FPakPlatformFile*>(&FPlatformFileManager::Get().GetPlatformFile());
		}
		else
		{
			PakPlatformFile = new FPakPlatformFile ;
			if(!PakPlatformFile->Initialize(&FPlatformFileManager::Get().GetPlatformFile(),TEXT("")))
			{
				UE_LOG(LogTemp, Error, TEXT("FPakPlatformFile failed to initialize"));
				return false;
			}
			FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
		}
	}
	FString PakFilePathFull = FPaths::ConvertRelativePathToFull(PakFileName);
	FPakFile* PakFile  = new FPakFile(PakPlatformFile, *PakFilePathFull, false);
	TArray<FString> FileList;
	FString packName = FPaths::GetBaseFilename(PakFileName);
	FString MountPoint = PakFile->GetMountPoint();
	PakFile->FindPrunedFilesAtPath(FileList, *MountPoint, true, false, true);
	PakFile->Release();
	FStreamableManager StreamableManager;
	for (int32 k = 0; k < FileList.Num(); k++)
	{
		FString AssetName = FileList[k];
		if (AssetName.Contains("BP_") && AssetName.Contains(".uasset"))
		{
			UE_LOG(LogTemp, Log, TEXT("Loading Pak: %s from File %s..."), *packName, *FileList[k]);
			FString AssetShortName = FPackageName::GetShortName(AssetName);
			FString FileName, FileExt;
			AssetShortName.Split(TEXT("."), &FileName, &FileExt);
			FString NewAssetName = FString("/") + packName + "/" + FileName + TEXT(".") + FileName + "_C";
			UE_LOG(LogTemp, Log, TEXT("Loading Asset %s ..."), *NewAssetName);
			UObject * objectToLoad = nullptr;
			FStringAssetReference ref = FStringAssetReference(NewAssetName);
			UE_LOG(LogTemp, Warning, TEXT("load object..."));
			objectToLoad = ref.TryLoad();
			if (objectToLoad)
			{
	            UBlueprintGeneratedClass* generatedClass = Cast<UBlueprintGeneratedClass>(objectToLoad);
	            UMapInfoAsset * mapInfoObject = Cast<UMapInfoAsset>(generatedClass->GetDefaultObject());
	            // Take default object by "info" class
	            if (mapInfoObject)
	            {
		            _mapsInfo.Add(mapInfoObject->_mapInfo);
		            UE_LOG(LogTemp, Warning, TEXT("Add data %s"), *mapInfoObject->GetFullName());
	            }
	            else
	            {
            		UE_LOG(LogTemp, Log, TEXT("File %s loading error!"), *FileList[k]);
	            }
			}
		}
	}
	return true;
}

UClass* UDLCLoader::LoadClassFromDLC(const FString& Filename)
{
	const FString Name = Filename + TEXT(".") +FPackageName::GetShortName(Filename) + TEXT("_C");
	return StaticLoadClass(UObject::StaticClass(), nullptr, *Name);
}

FPakPlatformFile* UDLCLoader::GetDLCFile()
{
	if (!DLCFile)
	{
		IPlatformFile *CurrentPlatformFile = FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"));
		if (CurrentPlatformFile)
		{
			DLCFile = static_cast<FPakPlatformFile*>(CurrentPlatformFile);
		}
		else
		{
			DLCFile = new FPakPlatformFile();
			ensure(DLCFile != nullptr);
			IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
#if UE_BUILD_SHIPPING == 0
			OriginalPlatformFile = &PlatformFile;
#endif
			if (DLCFile->Initialize(&PlatformFile, TEXT("")))
			{
				FPlatformFileManager::Get().SetPlatformFile(*DLCFile);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("DLCFile initialization error!"));
			}
		}
	}
	ensure(DLCFile != nullptr);
	return DLCFile;
}
