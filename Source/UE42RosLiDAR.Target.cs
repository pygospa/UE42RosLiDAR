// Copyright 2018 Kannan Thambiah. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE42RosLiDARTarget : TargetRules
{
	public UE42RosLiDARTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "UE42RosLiDAR" } );
	}
}
