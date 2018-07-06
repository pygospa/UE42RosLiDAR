// Copyright 2018 Kannan Thambiah. All rights reserved.

#include "LidarActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ConstructorHelpers.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"

const FString ALidarActor::ROSMsgType = TEXT("sensor_msgs/LaserScan");
const float ALidarActor::Degree2Radian = 0.01745329f;

ALidarActor::ALidarActor() :
	bShowDebugLog(false), IPv4Address("127.0.0.1"), Port(9090), ROSTopic("UE4LaserScan"), TickCount(0), TimePassed(0)
{
	PrimaryActorTick.bCanEverTick = true;

	SCSResolution = 1080;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("LiDAR"));

	DepthCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DepthCapture"));
	DepthCapture->SetupAttachment(RootComponent);
	DepthCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth; // SceneDepth in R
	DepthCapture->TextureTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("DepthTarget"));
	DepthCapture->TextureTarget->InitAutoFormat(SCSResolution,1);			// Scanner is 1 pixel high

	DepthImage.AddUninitialized(SCSResolution);
}

void ALidarActor::BeginPlay()
{
	Super::BeginPlay();

	// Set up UE4 rosbridge handler and publisher and establish connection
	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(IPv4Address, Port));
	Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic, ROSMsgType));
	Handler->AddPublisher(Publisher);
	Handler->Connect();

	// Create a LaserScan Object to send to ROS
	ScanData = MakeShareable(new sensor_msgs::LaserScan());
	ScanData->SetAngleMin(-1 * ScanAngleRad / 2.0);
	ScanData->SetAngleMax(ScanAngleRad / 2.0);
	ScanData->SetAngleIncrement(AngularResInRad);
	ScanData->SetRangeMin(MinimumDistance);
	ScanData->SetRangeMax(MaximumDistance);
}

void ALidarActor::EndPlay(const EEndPlayReason::Type Reason)
{
	Handler->Disconnect();
	Super::EndPlay(Reason);
}

void ALidarActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// check if it's time for another scan
	TimePassed += DeltaTime;
	if(TimePassed < TimePerScan) {
		return;
	}
	TimePassed = 0;

	// Get depth image
	DepthCapture->CaptureScene();
	FTextureRenderTargetResource *RenderTragetResource = (DepthCapture->TextureTarget)->GameThread_GetRenderTargetResource();
	RenderTragetResource->ReadFloat16Pixels(DepthImage);

	// Process depth data
	TArray<float> ProcessedDepthImage;
	ProcessedDepthImage.AddUninitialized(SCSResolution);
	FFloat16Color const *PixelPtr = DepthImage.GetData();
	for (int i = 0; i < DepthImage.Num(); ++i)
	{
		if ((PixelPtr->R.Encoded)/100 > MinimumDistance && (PixelPtr->R.Encoded)/100 < MaximumDistance)
		{
			ProcessedDepthImage.Add((PixelPtr->R.Encoded)/100);
		}
    else
		{
			ProcessedDepthImage.Add(0);
		}
		PixelPtr++;
	}

	// pack data into LaserMsg object
	ScanData->SetHeader(std_msgs::Header(++TickCount, FROSTime(), TEXT("0")));
	ScanData->SetRanges(ProcessedDepthImage);

	PrintDebugLog(FString::Printf(TEXT("Sending:\n\t")));
	PrintDebugLog(ScanData->ToString());

	Handler->PublishMsg(ROSTopic, ScanData);
	Handler->Process();
}

void ALidarActor::PrintDebugLog(FString string) const
{
	if (GEngine && bShowDebugLog)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, string);
	}
}

#if WITH_EDITOR
void ALidarActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Get all Laser Scanner components
	TArray<UActorComponent*> LidarComponents;
	GetComponents(LidarComponents);

	// Get name of the property that was changed
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// GET_MEMBER_NAME_CHECKED prevents future property name changes, as they will result in compilation error
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, bShowDebugLog))
	{
		if(bShowDebugLog)
		{
			PrintDebugLog(TEXT("Debugging switched on"));
		}
		else
		{
			bShowDebugLog = true;
			PrintDebugLog(TEXT("Debugging switched off"));
			bShowDebugLog = false;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, ScanAngleDeg))
	{
		ScanAngleRad = ScanAngleDeg * Degree2Radian;
		DepthCapture->FOVAngle = ScanAngleDeg;

		PrintDebugLog(FString::Printf(TEXT("ScanAngleDeg changed, calculated\n\t- ScanAngleRad: %f\n"), ScanAngleRad));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, ScanAngleRad))
	{
		ScanAngleDeg = ScanAngleRad / Degree2Radian;
		DepthCapture->FOVAngle = ScanAngleDeg;
		PrintDebugLog(FString::Printf(TEXT("ScanAngleRad changed, calculated\n\t- ScanAngleDeg: %f\n"), ScanAngleRad));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, AngularResInDeg))
	{
		AngularResInRad = AngularResInDeg * Degree2Radian;
		AngularResStepsInside = ScanAngleDeg / AngularResInDeg;
		AngularResStepsOf360 = 360.0 / AngularResInDeg;
		PrintDebugLog(FString::Printf(
					TEXT("AngularResInDeg changed, calculated\n\t- AngularResInRad: %f\n\t- AngularResStepsInside: %f \
						\n\t- AngularResStepsOf360: %f\n\t"),
					AngularResInRad, AngularResStepsInside, AngularResStepsOf360));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, AngularResInRad))
	{
		AngularResInDeg = AngularResInRad / Degree2Radian;
		AngularResStepsInside = ScanAngleDeg / AngularResInDeg;
		AngularResStepsOf360 = 360.0 / AngularResInDeg;
		PrintDebugLog(FString::Printf(
					TEXT("AngularResInDeg changed, calculated\n\t- AngularResInDeg: %f\n\t- AngularResStepsInside: %f \
						\n\t- AngularResStepsOf360: %f\n\t"),
					AngularResInDeg, AngularResStepsInside, AngularResStepsOf360));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, AngularResStepsInside))
	{
		AngularResInDeg = ScanAngleDeg / AngularResStepsInside;
		AngularResStepsOf360 = 360.0 / AngularResInDeg;
		PrintDebugLog(FString::Printf(
					TEXT("AngularResStepsInside changed, calculated\n\t- AngularResInDeg: %f\n\t- AngularResStepsOf360: %f\n\t"),
					AngularResInDeg, AngularResStepsOf360));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, AngularResStepsOf360))
	{
		AngularResInDeg = 360.0 / AngularResStepsOf360;
		AngularResStepsInside = ScanAngleDeg / AngularResInDeg;
		PrintDebugLog(FString::Printf(
					TEXT("AngularResStepsInside changed, calculated\n\t- AngularResInDeg: %f\n\t- AngularResStepsInside: %f\n\t"),
					AngularResInDeg, AngularResStepsInside));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, MinimumDistance))
	{
		PrintDebugLog(FString::Printf(TEXT("MinimumDistance changed to %f\n"), MinimumDistance));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, MinimumDistance))
	{
		PrintDebugLog(FString::Printf(TEXT("MaximumDistance changed to %f\n"), MaximumDistance));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALidarActor, TimePerScan))
	{
		PrintDebugLog(FString::Printf(TEXT("TimePerScan changed to %f\n"), TimePerScan));
	}
}
#endif
