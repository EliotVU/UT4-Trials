#include "Trials.h"
#include "TrialsGameState.h"
#include "TrialsPlayerState.h"

#include "UTHUDWidget_Objective.h"

UUTHUDWidget_Objective::UUTHUDWidget_Objective(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ScreenPosition = FVector2D(0.5f, 0.0f);
    Size = FVector2D(0.f, 0.f);
    Origin = FVector2D(0.5f, 0.5f);
    InWorldAlpha = 0.8f;
    MaxIconScale = 0.75f;

    TopEdgePadding = 0.1f;
    BottomEdgePadding = 0.18f;
    LeftEdgePadding = 0.025f;
    RightEdgePadding = 0.025f;

    static ConstructorHelpers::FObjectFinder<USoundBase> TickSoundFinder(TEXT("SoundWav'/Trials/CharFade.CharFade'"));
    TickSound = TickSoundFinder.Object;

    static ConstructorHelpers::FObjectFinder<USoundBase> EndTickSoundFinder(TEXT("SoundWav'/Trials/CharFade.CharFade'"));
    EndTickSound = EndTickSoundFinder.Object;

    StatusText.Font = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->MediumFont;
    StatusText.bDrawShadow = true;
    StatusText.ShadowDirection = FVector2D(1.f, 2.f);
    StatusText.ShadowColor = FLinearColor::Black;

    RecordText.Font = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->MediumFont;
    RecordText.HorzPosition = ETextHorzPos::Right;
    RecordText.bDrawShadow = true;
    RecordText.ShadowDirection = FVector2D(1.f, 2.f);
    RecordText.ShadowColor = FLinearColor::Black;

    TimerText.Font = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->LargeFont;
    TimerText.Position.Y = 32;
    TimerText.bDrawShadow = true;
    TimerText.ShadowDirection = FVector2D(1.f, 2.f);
    TimerText.ShadowColor = FLinearColor::Black;
}

void UUTHUDWidget_Objective::InitializeWidget(AUTHUD* Hud)
{
    Super::InitializeWidget(Hud);
}

void UUTHUDWidget_Objective::Draw_Implementation(float DeltaTime)
{
    auto* GameState = Cast<ATrialsGameState>(UTGameState);
    if (GameState == nullptr) return;

    FVector ViewPoint;
    FRotator ViewRotation;

    UTPlayerOwner->GetPlayerViewPoint(ViewPoint, ViewRotation);
    DrawIndicators(GameState, ViewPoint, ViewRotation, DeltaTime);
    DrawStatus(GameState, DeltaTime);
}

void UUTHUDWidget_Objective::DrawIndicators(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, float DeltaTime)
{
    auto* PS = Cast<ATrialsPlayerState>(UTPlayerOwner->PlayerState);
    if (!PS) return;

    for (int32 i = 0; i < GameState->Objectives.Num(); ++i)
    {
        auto* target = GameState->Objectives[i];
        if (target && (PS->ActiveObjectiveInfo == nullptr || target->ObjectiveInfo == PS->ActiveObjectiveInfo))
        {
            DrawObjWorld(GameState, PlayerViewPoint, PlayerViewRotation, target);
        }
    }
}

void UUTHUDWidget_Objective::DrawStatus(ATrialsGameState* GameState, float DeltaTime)
{
    auto* ViewedPawn = Cast<APawn>(UTPlayerOwner->GetViewTarget());
    auto* ViewPS = ViewedPawn ? Cast<ATrialsPlayerState>(ViewedPawn->PlayerState) : nullptr;
    auto* OwnerPS = ViewPS ? ViewPS : Cast<ATrialsPlayerState>(UTPlayerOwner->UTPlayerState);
    if (OwnerPS != nullptr)
    {
        auto* Target = OwnerPS->ActiveObjectiveInfo;
        if (Target != nullptr)
        {
            FText TitleText = Target->Title;
            StatusText.Text = TitleText;
            RenderObj_Text(StatusText);

            FText TimeText = ATrialsTimerState::FormatTime(Target->RecordTime);
            RecordText.RenderColor = FLinearColor::Yellow;
            RecordText.Text = TimeText;
            RenderObj_Text(RecordText);
        }

        auto* TimerState = OwnerPS->TimerState;
        if (Target != nullptr && TimerState != nullptr)
        {
            bool IsActive = TimerState->State == TS_Active;

            float RecordTime = TimerState->GetRecordTime();
            float Timer = TimerState->GetRemainingTime();

            FLinearColor TimerColor = !IsActive
                ? FLinearColor::Yellow
                : ATrialsTimerState::GetTimerColor(Timer);

            FText TimeText = ATrialsTimerState::FormatTime(IsActive ? Timer : RecordTime);

            AnimationAlpha += (DeltaTime * 3);

            float Alpha = FMath::Sin(AnimationAlpha);
            Alpha = FMath::Abs<float>(Alpha);

            TimerText.RenderColor = TimerColor;
            TimerText.RenderOpacity = Alpha;
            TimerText.Text = TimeText;
            RenderObj_Text(TimerText);

            if (IsActive)
            {
                // HACK: + 1 to distinguish from -0.99 +0.99, both are rounded to 0.
                bool bTick = Timer < 10.0f && static_cast<int32>(LastDrawnTimer + 1) != static_cast<int32>(Timer + 1);
                if (bTick)
                {
                    if (Timer < 0.0f && static_cast<int32>(Timer) == 0)
                    {
                        UGameplayStatics::PlaySound2D(GetWorld(), EndTickSound, 3.5f, 1.0f, 0.0f);
                    }
                    else if (Timer > 0.0f)
                    {
                        UGameplayStatics::PlaySound2D(GetWorld(), TickSound, 1.5f, 20.0f / Timer, 0.0f);
                    }
                }
                LastDrawnTimer = Timer;
            }
        }
        else
        {
            AnimationAlpha = 0.0;
        }
    }
}

void UUTHUDWidget_Objective::DrawObjWorld(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, ATrialsObjective* Target)
{
    bScaleByDesignedResolution = false;

    bool bSpectating = UTPlayerOwner->PlayerState && UTPlayerOwner->PlayerState->bOnlySpectator;

    IconTemplate.RenderColor = FLinearColor::Yellow;
    CameraIconTemplate.RenderColor = IconTemplate.RenderColor;

    // Draw the flag / flag base in the world
    float Dist = (Target->GetActorLocation() - PlayerViewPoint).Size();
    float WorldRenderScale = RenderScale * MaxIconScale;

    bool bDrawInWorld = false;
    bool bDrawEdgeArrow = false;

    FVector DrawScreenPosition;
    FVector WorldPosition = Target->GetActorLocation() + FVector(0.f, 0.f, 128 * 3.f);

    float OldAlpha = IconTemplate.RenderOpacity;

    FVector ViewDir = PlayerViewRotation.Vector();

    float Edge = CircleTemplate.GetWidth()*WorldRenderScale;
    bool bShouldDrawIcon = true;

    if ((bSpectating || bShouldDrawIcon))
    {
        WorldPosition = Target->GetActorLocation();
        WorldPosition += FVector(0.f, 0.f, 128 * 0.75f);
        
        bDrawInWorld = true;
        DrawScreenPosition = GetAdjustedScreenPosition(WorldPosition, PlayerViewPoint, ViewDir, Dist, Edge, bDrawEdgeArrow);
    }
    else
    {
        DrawScreenPosition = GetAdjustedScreenPosition(WorldPosition, PlayerViewPoint, ViewDir, Dist, Edge, bDrawEdgeArrow);
    }

    // Look to see if we should be displaying the in-world indicator for the flag.
    if (bDrawInWorld)
    {
        float PctFromCenter = (DrawScreenPosition - FVector(0.5f*GetCanvas()->ClipX, 0.5f*GetCanvas()->ClipY, 0.f)).Size() / GetCanvas()->ClipX;
        float CurrentWorldAlpha = InWorldAlpha * FMath::Min(6.f*PctFromCenter, 1.f);

        // don't overlap player beacon
        UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
        float X, Y;
        float Scale = Canvas->ClipX / 1920.f;
        Canvas->TextSize(TinyFont, FString("+999   A999"), X, Y, Scale, Scale);

        if (!bDrawEdgeArrow)
        {
            float MinDistSq = FMath::Square(0.06f*GetCanvas()->ClipX);
            float ActualDistSq = FMath::Square(DrawScreenPosition.X - 0.5f*GetCanvas()->ClipX) + FMath::Square(DrawScreenPosition.Y - 0.5f*GetCanvas()->ClipY);
            if (ActualDistSq > MinDistSq)
            {
                DrawScreenPosition.Y -= 1.5f*Y;
            }
        }
        DrawScreenPosition.X -= RenderPosition.X;
        DrawScreenPosition.Y -= RenderPosition.Y;

        TitleItem.RenderOpacity = CurrentWorldAlpha;
        DetailsItem.RenderOpacity = CurrentWorldAlpha;
        IconTemplate.RenderOpacity = CurrentWorldAlpha;
        CircleTemplate.RenderOpacity = CurrentWorldAlpha;
        CircleBorderTemplate.RenderOpacity = CurrentWorldAlpha;

        float InWorldFlagScale = WorldRenderScale;
        RenderObj_TextureAt(CircleTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleTemplate.GetWidth()* InWorldFlagScale, CircleTemplate.GetHeight()* InWorldFlagScale);
        RenderObj_TextureAt(CircleBorderTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleBorderTemplate.GetWidth()* InWorldFlagScale, CircleBorderTemplate.GetHeight()* InWorldFlagScale);

        if (bDrawEdgeArrow)
        {
            DrawEdgeArrow(WorldPosition, PlayerViewPoint, PlayerViewRotation, DrawScreenPosition, CurrentWorldAlpha, WorldRenderScale);

            FFormatNamedArguments Args;
            FText NumberText = FText::AsNumber(int32(0.01f*Dist));
            Args.Add("Dist", NumberText);

            DetailsItem.Text = FText::Format(NSLOCTEXT("Trials", "ObjectiveDetails", "{Dist}m"), Args);
            RenderObj_Text(DetailsItem, FVector2D(DrawScreenPosition));
        }
        else
        {
            auto* ViewTarget = Cast<AUTCharacter>(UTPlayerOwner->GetViewTarget());
            auto* ViewPS = Cast<ATrialsPlayerState>(ViewTarget ? ViewTarget->PlayerState : UTPlayerOwner->PlayerState);
            if (ViewPS == nullptr) return;

            if (ViewPS->ActiveObjectiveInfo == nullptr)
            {
                TitleItem.Text = Target->ObjectiveInfo->Title;
                RenderObj_Text(TitleItem, FVector2D(DrawScreenPosition));
            }
            else if (ViewPS->ActiveObjectiveInfo == Target->ObjectiveInfo)
            {
                auto* TimerState = ViewPS->TimerState;
                if (TimerState != nullptr)
                {
                    bool IsActive = TimerState->State == TS_Active;
                    float RecordTime = TimerState->GetRecordTime();
                    float Timer = TimerState->GetRemainingTime();

                    FLinearColor TimerColor = IsActive
                        ? ATrialsTimerState::GetTimerColor(Timer)
                        : FLinearColor::Yellow;
                    DetailsItem.RenderColor = TimerColor;

                    FText TimeText = ViewPS->FormatTime(IsActive ? Timer : RecordTime);
                    DetailsItem.Text = TimeText;
                    RenderObj_Text(DetailsItem, FVector2D(DrawScreenPosition));
                }
            }
        }
        RenderObj_TextureAt(IconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, 1.25f*IconTemplate.GetWidth()*WorldRenderScale, 1.25f*IconTemplate.GetHeight()* WorldRenderScale);
    }
    TitleItem.RenderOpacity = OldAlpha;
    DetailsItem.RenderOpacity = OldAlpha;
    IconTemplate.RenderOpacity = OldAlpha;
    CircleTemplate.RenderOpacity = 1.f;
    CircleBorderTemplate.RenderOpacity = 1.f;

    bScaleByDesignedResolution = true;
}

FVector UUTHUDWidget_Objective::GetAdjustedScreenPosition(const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float IconSize, bool& bDrawEdgeArrow)
{
    FVector DrawScreenPosition = GetCanvas()->Project(WorldPosition);
    FVector FlagDir = (WorldPosition - ViewPoint).GetSafeNormal();
    if ((ViewDir | FlagDir) < 0.f)
    {
        bDrawEdgeArrow = true;
        FVector LeftDir = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
        bool bLeftOfScreen = (FlagDir | LeftDir) > 0.f;
        DrawScreenPosition.X = bLeftOfScreen ? -0.5f*GetCanvas()->ClipX * (ViewDir | FlagDir) : GetCanvas()->ClipX * (1.f + 0.5f*(ViewDir | FlagDir));
        DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY - IconSize;
        DrawScreenPosition.Z = 0.0f;
        DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
        return DrawScreenPosition;
    }
    if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > GetCanvas()->ClipX))
    {
        bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
        float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - GetCanvas()->ClipX;
        bDrawEdgeArrow = true;
        DrawScreenPosition.X = bLeftOfScreen ? IconSize + LeftEdgePadding*GetCanvas()->ClipX : GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX;
        //Y approaches 0.9*Canvas->ClipY as further off screen
        float MaxOffscreenDistance = GetCanvas()->ClipX;
        DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) * (DrawScreenPosition.Y - (1.f - BottomEdgePadding)*GetCanvas()->ClipY);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
    }
    else
    {
        DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
        DrawScreenPosition.Z = 0.0f;
    }
    return DrawScreenPosition;
}

void UUTHUDWidget_Objective::DrawEdgeArrow(FVector InWorldPosition, FVector PlayerViewPoint, FRotator PlayerViewRotation, FVector InDrawScreenPosition, float CurrentWorldAlpha, float WorldRenderScale)
{
    ArrowTemplate.RenderScale = 1.1f * WorldRenderScale;
    ArrowTemplate.RenderOpacity = CurrentWorldAlpha;
    ArrowTemplate.RenderColor = FColor::Yellow;
    float RotYaw = FMath::Acos(PlayerViewRotation.Vector() | (InWorldPosition - PlayerViewPoint).GetSafeNormal()) * 180.f / PI;
    if (InDrawScreenPosition.X < 0.f)
    {
        RotYaw *= -1.f;
    }
    RenderObj_TextureAtWithRotation(ArrowTemplate, FVector2D(InDrawScreenPosition.X, InDrawScreenPosition.Y), RotYaw);
}