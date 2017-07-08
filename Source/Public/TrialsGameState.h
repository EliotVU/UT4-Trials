#pragma once

#include "TrialsObjective.h"
#include "TrialsGameMode.h"

#include "TrialsGameState.generated.h"

UCLASS()
class ATrialsGameState : public AUTGameState
{
    GENERATED_UCLASS_BODY()

    // TODO: Refactor to Objective Targets and add a separate Objectives array pointing to ObjectiveInfos'
    UPROPERTY(Replicated, BlueprintReadOnly, Category = Trials, DisplayName = "Objective Targets")
    TArray<ATrialsObjective*> Objectives;

    void AddTarget(ATrialsObjective* Obj)
    {
        Objectives.AddUnique(Obj);
    }

    void RemoveTarget(ATrialsObjective* Obj)
    {
        Objectives.Remove(Obj);
    }

    bool AllowMinimapFor(AUTPlayerState* PS) override;
    bool OnSameTeam(const AActor* Actor1, const AActor* Actor2) override;
};
