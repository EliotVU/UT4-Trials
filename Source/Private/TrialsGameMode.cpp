#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsPlayerState.h"
#include "TrialsHUD.h"
#include "TrialsObjectiveCompleteMessage.h"

ATrialsGameMode::ATrialsGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ATrialsGameState::StaticClass();
	PlayerStateClass = ATrialsPlayerState::StaticClass();
    HUDClass = ATrialsHUD::StaticClass();
}

bool ATrialsGameMode::AllowSuicideBy(AUTPlayerController* PC)
{
    ATrialsPlayerState* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return ScorerPS->ActiveObjectiveInfo != nullptr || Super::AllowSuicideBy(PC);
}

AActor* ATrialsGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    // TODO: If reached Hub once, give player a spawn with Tag "Hub".
    // TODO: If player has hub and chosen a objective, spawn player at the objective's set PlayerStart.
    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(Player->PlayerState);
    if (PS->ActiveObjectiveInfo != nullptr)
    {
        return PS->ActiveObjectiveInfo->GetPlayerSpawn(Player);
    }
    return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ATrialsGameMode::ScoreTrialObjective(AUTPlayerController* PC, ATrialsObjectiveInfo* objInfo)
{
    if (PC == nullptr || objInfo == nullptr) return;

    // TODO: Add event here

    // We don't want to complete an objective for clients whom have already completed or are doing a different objective.
    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (!ScorerPS->bIsObjectiveTimerActive || ScorerPS->ActiveObjectiveInfo != objInfo) return;

    int32 RecordSwitch;
    float Timer = ScorerPS->EndObjectiveTimer();
    float RecordTime = objInfo->RecordTime;
    if (Timer < RecordTime || RecordTime <= 0.00) // New all time
    {
        // New top record!
        RecordSwitch = 0;
        objInfo->RecordTime = Timer;
        ScorerPS->ObjectiveRecordTime = Timer;
    }
    else if (Timer == RecordTime) // Tied with all time
    {
        // tie or slower record...
        RecordSwitch = 1;
    }
    else // worse, check personal time
    {
        RecordTime = ScorerPS->ObjectiveRecordTime;
        // New or first personal record
        if (RecordTime <= 0.00)
        {
            RecordSwitch = 3;
            ScorerPS->ObjectiveRecordTime = Timer;
        }
        else if (Timer < RecordTime)
        {
            RecordSwitch = 4;
            ScorerPS->ObjectiveRecordTime = Timer;
        }
        else if (Timer == RecordTime)
        {
            RecordSwitch = 1;
        }
        else
        {
            RecordSwitch = 2;
        }
    }


    // ...New time?!
    BroadcastLocalized(this, UTrialsObjectiveCompleteMessage::StaticClass(), RecordSwitch, ScorerPS, nullptr, objInfo);

    // TODO: Add record event here
}
