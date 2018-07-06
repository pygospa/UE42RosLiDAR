// Copyright 2018 Kannan Thambiah. All rights reserved.

#pragma once

#include "sensor_msgs/LaserScan.h"
#include "CoreMinimal.h"
#include "LidarActor.generated.h"

class AActor;
class FROSBridgeHandler;
class FROSBridgePublisher;
class SceneCaptureComponent2D;

/**
 * A LidarActor simmulates a LIght Detection and Ranging device (commonly known as laser scanner) by using the
 * SceneCaptureComponent2D together with the SCS_SceneDepth scene capturing source of Unreal Engine 4. After capturing
 * the depth data, it is send to a ROS instance using ROS' rosbridge_suite, as well as Unreals the URosBridge plugin
 */
UCLASS()
class LIDAR_API ALidarActor : public AActor
{
	GENERATED_BODY()

		// --- Methods --- //
public:
	/**
	 * Default constructor for ALidarActor
	 */
	ALidarActor();

	/**
	 * Called every frame to grab the scene data and send it over to ROS
	 */
	virtual void Tick(float DeltaTime) override;

	/*
	 * Called when a property is changed from the Editor to recalculate dependencies and produce debugging output
	 */
	void PostEditChangeProperty(FPropertyChangedEvent & PropretyChangedEvent) override;

protected:
	/**
	 * Called when the game starts to open a connection to a rosbridge server via a rosbridge websocket
	 */
	virtual void BeginPlay() override;

	/**
	 * Called when the game stops to close the connection to the rosbridge server that was opened at BeginPlay()
	 */
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	/**
	* Method helping pritning out debug messages
	*/
	void PrintDebugLog(FString string) const;

	// --- Variables --//
public:
	/**
	* Flag to controll if debugging output is printed on screen
	*/
	UPROPERTY(EditAnywhere, Category = "LiDAR|Debugging")
		bool bShowDebugLog;

	/**
	 * The IPv4 address of the computer running the rosbridge server. Default value is 127.0.0.1
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|ROSBridge", meta = (DisplayName = "IPv4 Address"))
		FString IPv4Address;

	/**
	 * The port number that the rosbridge server is listening to. According to protocol port 0 is reserved, and the port
	 * part of an address has 16 bit --> 65535 as max port number. Default port for rosbridge websocket is 9090.
	 * @see https://en.wikipedia.org/wiki/Port_(computer_networking)
	 */
	UPROPERTY(EditAnywhere, Category = "LiDar|ROSBridge", meta = (ClampMin = "1", ClampMax = "65535"))
		uint32 Port;

	/**
	 * The ros topic that the actor is publishing to. Default value is "UE4LaserScan"
	 * @see https://wiki.ros.org/ROS/Tutorials/UnderstandingTopics
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|ROSBridge", meta = (DisplayName = "ROSTopic"))
		FString ROSTopic;

	/**
	 * The angle of the area covered by the laser scanner in degrees
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "360.0", DisplayName = "Scanning Angle in Degrees"))
		float ScanAngleDeg;

	/**
	 * The angle of the area covered by the laser scanner in radians
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "6.2831853", DisplayName = "Scanning Angle in Radians"))
		float ScanAngleRad;

	/**
	 * The angular resolution of the laser scanner in degrees
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "360.0", DisplayName = "Angular Resolution in Degrees"))
		float AngularResInDeg;

	/**
	 * The angular resolution of the laser scanner in radians
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "360.0", DisplayName = "Angular Resolution in Radians"))
		float AngularResInRad;

	/**
	 * The angular resolution of the laser scanner in steps inside the area covered by the scanner
	 * 360°/2^16 = 0.005° - highest precision LiDAR yields a 0.07 precision - so we should be good :)
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "360.0", DisplayName = "Angular Resolution in Steps inside Scanning Angle"))
		uint16 AngularResStepsInside;

	/**
	 * The angular resolution of the laser scanner in steps inside a full 360° angle (even if the scanner does not cover a
	 * full 360° angle
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", ClampMax = "360.0", DisplayName = "AngularResolution in Steops of 360°"))
		uint16 AngularResStepsOf360;

	/**
	 * Minimum distance that an object has to be away from the sensor to get realized
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", DisplayName="Minimum Distance in meter"))
		float MinimumDistance;

	/**
	 * Maximum distance that an object can be away from the sensor and still be realized
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", DisplayName="Maximum Distance in meter"))
		float MaximumDistance;

	/**
	 * Maximum distance that an object can be away from the sensor and still be realized
	 */
	UPROPERTY(EditAnywhere, Category = "LiDAR|Scanning Specs",
			meta = (ClampMin = "0.0", DisplayName="Time between Measurements in seconds"))
		float TimePerScan;

private:
	/**
	 * RosBridgeHandler
	 */
	TSharedPtr<FROSBridgeHandler> Handler;

	/**
	 * ROSBridgePublisher
	 */
	TSharedPtr<FROSBridgePublisher> Publisher;

	/**
	 * LaserScan message object containting the data of the scan
	 */
	TSharedPtr<sensor_msgs::LaserScan> ScanData;

	/**
	 * Counts every tick as this information is needed in the message header for ROS
	 */
	uint32 TickCount;

	/**
	 * Cummulated time passed between ticks
	 */
	float TimePassed;

	// --- Constants ---//
	/**
	 * Message type to send via ROSBridge
	 */
	static const FString ROSMsgType;

	/**
	 * Constant to calculate form angles in degree to angles in radians
	 * Eg. 90° <=> 1.57
	 */
	static const float Degree2Radian;

	/**
	 * Camara capture component for depth image
	 * TODO: This is accessible for debugging purposes - should be removed once finished
	 */
	UPROPERTY(EditAnywhere, Category = "LaserScanner|Cam")
		USceneCaptureComponent2D* DepthCapture;

	/*
	 * Buffers for reading the data from the gpu
	 */
	TArray<FFloat16Color> DepthImage;

	/*
	 * Scene Capture Source resolution
	 */
	uint32 SCSResolution;
};
