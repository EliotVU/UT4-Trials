#pragma once

#include "TrialsGameMode.h"
#include "TrialsObjectiveInfo.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
	GENERATED_UCLASS_BODY()

    float ObjectiveStartTime;
    float ObjectiveEndTime;
    bool bIsObjectiveTimerActive;

public:
    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjectiveInfo* ActiveObjectiveInfo;

    UPROPERTY(Replicated)
    float LastScoreObjectiveTimer;

    /* The player's record time for the current active objective. <= 0 == N/A */
    UPROPERTY(Replicated)
    float ObjectiveRecordTime;

    void StartObjectiveTimer()
    {
        bIsObjectiveTimerActive = true;
        ObjectiveEndTime = 0.00;
        ObjectiveStartTime = GetWorld()->RealTimeSeconds;
    }

    float EndObjectiveTimer()
    {
        ObjectiveEndTime = GetWorld()->RealTimeSeconds;
        LastScoreObjectiveTimer = GetObjectiveTimer();
        NetUpdateTime = GetWorld()->TimeSeconds - 1;
        bIsObjectiveTimerActive = false;
        return LastScoreObjectiveTimer;
    }

    float GetObjectiveTimer()
    {
        if (ActiveObjectiveInfo == nullptr) return -1;
        return RoundTime((bIsObjectiveTimerActive ? GetWorld()->RealTimeSeconds : ObjectiveEndTime) - ObjectiveStartTime);
    }

    float GetObjectiveRemainingTime()
    {
        return bIsObjectiveTimerActive 
            ? (ActiveObjectiveInfo ? ActiveObjectiveInfo->RecordTime : 0.00) - GetObjectiveTimer()
            : ObjectiveRecordTime;
    }

    void SetObjective(ATrialsObjectiveInfo* objectiveInfo)
    {
        ActiveObjectiveInfo = objectiveInfo;
    }

    float RoundTime(float time)
    {
        return roundf(time*100.0)/100.0;
    }

    FText FormatTime(float value)
    {
        float seconds = fabs(value);
        int32 minutes = (int32)seconds/60;
        int32 hours = minutes/60;
        seconds = seconds - (minutes*60.f);
        minutes = minutes - (hours*60);

        FString secondsString = FString::Printf(TEXT("%f"), seconds);
        secondsString = secondsString.Left(secondsString.Find(".") + 3).Append("s");
        if (minutes != 0 && seconds < 10.f) 
            secondsString = TEXT("0") + secondsString;

        FString minutesString = (hours != 0 && minutes < 10) 
            ? TEXT("0") + minutes 
            : FString::FromInt(minutes);

        FString hoursString = FString::FromInt(hours);

        FString output;
        if (hours != 0)
            output = hoursString + TEXT("h ");

        if (minutes != 0)
            output += minutesString + TEXT("m ");

        output = value < 0 ? TEXT("-") + output + secondsString : output + secondsString;
        return FText::FromString(output);
    }
};