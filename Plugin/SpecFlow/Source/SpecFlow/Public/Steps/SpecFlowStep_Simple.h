// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpecFlowStep.h"
#include "SpecFlowStep_Simple.generated.h"

/**
 * 
 */
UCLASS()
class SPECFLOW_API USpecFlowStep_Simple : public USpecFlowStep
{
	GENERATED_BODY()
public:
	typedef TFunction<bool()> StepRun;
	static TObjectPtr<USpecFlowStep_Simple> FromLambda(const FString& StepName, StepRun Func);

	virtual bool OnStepRun() override { return Run(); }
private:
	StepRun Run;
};
