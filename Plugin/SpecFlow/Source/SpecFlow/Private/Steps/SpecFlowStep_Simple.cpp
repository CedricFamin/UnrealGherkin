// Fill out your copyright notice in the Description page of Project Settings.


#include "Steps/SpecFlowStep_Simple.h"

TObjectPtr<USpecFlowStep_Simple> USpecFlowStep_Simple::FromLambda(const FString& StepName, StepRun Func)
{
	TObjectPtr<USpecFlowStep_Simple> NewStep = NewObject<USpecFlowStep_Simple>();
	NewStep->StepName = StepName;
	NewStep->HttpRoute = "SpecFlow/Steps/" + StepName;
	NewStep->Run = Func;
	return NewStep;
}
