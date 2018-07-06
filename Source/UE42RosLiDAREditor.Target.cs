// Copyright 2018 Kannan Thambiah. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE42RosLiDAREditorTarget : TargetRules
{
	public UE42RosLiDAREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "UE42RosLiDAR" } );
	}
}
