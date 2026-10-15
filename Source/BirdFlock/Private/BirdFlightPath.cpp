#include "BirdFlightPath.h"
#include "BirdFlockModule.h"
#include "FlockBird.h"
#include "Components/SplineComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "DrawDebugHelpers.h"

namespace
{
	// Fraction of birds that play the alternate animation when one is set.
	constexpr float AltAnimChance = 0.35f;
	// Min separation kept between the deactivate radius and the trigger radius.
	constexpr float DeactivateMinGap = 500.f;
}

ABirdFlightPath::ABirdFlightPath()
{
	PrimaryActorTick.bCanEverTick = true;
	// Distance polling does not need per-frame precision; birds tick themselves at full rate.
	PrimaryActorTick.TickInterval = 0.2f;

	FlightSpline = CreateDefaultSubobject<USplineComponent>(TEXT("FlightSpline"));
	SetRootComponent(FlightSpline);

	// Default preset: horizontal circle so a freshly placed actor is ready for circling flight.
	FlightSpline->ClearSplinePoints(false);
	constexpr int32 NumPresetPoints = 12;
	for (int32 i = 0; i < NumPresetPoints; ++i)
	{
		const float Angle = static_cast<float>(i) / NumPresetPoints * 2.f * PI;
		FlightSpline->AddSplinePoint(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 800.f,
			ESplineCoordinateSpace::Local, false);
	}
	FlightSpline->SetClosedLoop(true, false);
	FlightSpline->UpdateSpline();

	FlockAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlockAudio"));
	FlockAudio->SetupAttachment(FlightSpline);
	FlockAudio->bAutoActivate = false;
	FlockAudio->bAllowSpatialization = true;
}

void ABirdFlightPath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Loop mode flies the straight return segment end->start, which the spline provides
	// only when flagged closed; the other modes must bounce/stop on the real endpoints.
	FlightSpline->SetClosedLoop(FlightMode == EBirdFlightMode::Loop);

	// Applied here so per-instance edits re-arm the poll rate immediately.
	PrimaryActorTick.TickInterval = FMath::Clamp(PlayerCheckInterval, 0.05f, 60.f);

	// Seed up front so the trigger rolls differ between actor instances even
	// before the first flock activation.
	Rng.Initialize(RandomSeed != 0 ? RandomSeed : FMath::Rand());
}

float ABirdFlightPath::GetPlayerDistance() const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return -1.f;
	}
	return FVector::Dist(Player->GetActorLocation(), GetActorLocation());
}

float ABirdFlightPath::GetEffectiveDeactivateDistance() const
{
	return FMath::Max(DeactivateDistance, TriggerDistanceMax + DeactivateMinGap);
}

void ABirdFlightPath::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Distance = GetPlayerDistance();
	if (Distance < 0.f)
	{
		return;
	}

	if (bDrawDebugDistances)
	{
		const float LifeTime = FMath::Clamp(PlayerCheckInterval, 0.05f, 60.f) * 1.2f + 1.f;
		const FVector Center = GetActorLocation();
		DrawDebugSphere(GetWorld(), Center, TriggerDistanceMax, 24, FColor::Green, false, LifeTime, 0, 2.f);
		DrawDebugSphere(GetWorld(), Center, TriggerDistanceMin, 24, FColor::Orange, false, LifeTime, 0, 2.f);
		DrawDebugSphere(GetWorld(), Center, GetEffectiveDeactivateDistance(), 24, FColor::Red, false, LifeTime, 0, 2.f);
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			DrawDebugLine(GetWorld(), Center, Player->GetActorLocation(), FColor::Yellow, false, LifeTime, 0, 2.f);
			DrawDebugString(GetWorld(), Center + FVector(0.f, 0.f, 100.f),
				FString::Printf(TEXT("%.0f cm"), Distance), nullptr, FColor::White, LifeTime);
		}
	}

	switch (FlockState)
	{
	case EFlockState::Idle:
	{
		if (bTriggerOnlyOnce && bOneShotConsumed)
		{
			FlockState = EFlockState::DoneForever;
			break;
		}

		const bool bInBand = Distance <= TriggerDistanceMax && Distance >= TriggerDistanceMin;
		if (!bInBand)
		{
			// Reset so re-entering the band always gets a fresh attempt.
			RetryCountdown = 0.f;
			break;
		}

		RetryCountdown -= DeltaSeconds;
		if (RetryCountdown > 0.f)
		{
			break;
		}

		// TriggerRetryInterval <= 0 means a single attempt per approach.
		RetryCountdown = (TriggerRetryInterval > 0.f) ? TriggerRetryInterval : FLT_MAX;
		if (Rng.FRand() <= TriggerProbability)
		{
			ActivateFlock();
		}
		break;
	}

	case EFlockState::Active:
	{
		if (Distance > GetEffectiveDeactivateDistance())
		{
			DeactivateFlock(false);
			break;
		}

		if (AllBirdsFinished())
		{
			if (FlightDuration > 0.f)
			{
				// The flock departed on its own; a natural end of the visit, the
				// next appearance is a fresh trigger with its own probability roll.
				DeactivateFlock(true);
			}
			else if (FlightMode == EBirdFlightMode::SinglePass)
			{
				for (TObjectPtr<AFlockBird>& Bird : ActiveBirds)
				{
					if (Bird)
					{
						Bird->Destroy();
					}
				}
				ActiveBirds.Reset();

				if (FlockAudio && FlockAudio->IsPlaying())
				{
					FlockAudio->Stop();
				}

				if (bTriggerOnlyOnce)
				{
					FlockState = EFlockState::DoneForever;
				}
				else
				{
					FlockState = EFlockState::WaitingRespawn;
					RespawnCountdown = FMath::Max(SinglePassRespawnDelay, 0.f);
				}
			}
		}
		break;
	}

	case EFlockState::WaitingRespawn:
	{
		if (Distance > GetEffectiveDeactivateDistance())
		{
			FlockState = (bTriggerOnlyOnce || !bRetriggerAfterDeactivate) ? EFlockState::DoneForever : EFlockState::Idle;
			RetryCountdown = 0.f;
			break;
		}

		if (Distance <= TriggerDistanceMax)
		{
			RespawnCountdown -= DeltaSeconds;
			if (RespawnCountdown <= 0.f)
			{
				// The flock already won its roll; respawn without rolling again.
				ActivateFlock();
			}
		}
		break;
	}

	case EFlockState::DoneForever:
		break;
	}
}

bool ABirdFlightPath::AllBirdsFinished() const
{
	for (const TObjectPtr<AFlockBird>& Bird : ActiveBirds)
	{
		if (Bird && !Bird->bFinished)
		{
			return false;
		}
	}
	return true;
}

void ABirdFlightPath::ActivateFlock()
{
	Rng.Initialize(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	USkeletalMesh* Mesh = BirdMesh.LoadSynchronous();
	if (!Mesh)
	{
		UE_LOG(LogBirdFlock, Warning,
			TEXT("BirdFlightPath '%s': BirdMesh is not set, flock skipped. / Setka ptitsy ne zadana."),
			*GetName());
		return;
	}

	UClass* AnimClassToUse = nullptr;
	if (UClass* LoadedAnimClass = BirdAnimClass.LoadSynchronous())
	{
		if (LoadedAnimClass->IsChildOf(UAnimInstance::StaticClass()))
		{
			AnimClassToUse = LoadedAnimClass;
		}
		else
		{
			UE_LOG(LogBirdFlock, Warning,
				TEXT("BirdFlightPath '%s': BirdAnimClass is not a UAnimInstance, falling back to single-node animations."),
				*GetName());
		}
	}

	UAnimationAsset* MainAnim = nullptr;
	UAnimationAsset* AltAnim = nullptr;
	if (!AnimClassToUse)
	{
		MainAnim = BirdAnimToPlay.LoadSynchronous();
		AltAnim = BirdAnimToPlayAlt.LoadSynchronous();
		if (!MainAnim && !AltAnim)
		{
			UE_LOG(LogBirdFlock, Warning,
				TEXT("BirdFlightPath '%s': no BirdAnimToPlay/BirdAnimClass, birds will be static. / Ne zadana animatsiya."),
				*GetName());
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SplineLength = FlightSpline->GetSplineLength();
	ActiveBirds.Reset();

	// Resolve sound attenuation once: asset override or runtime sphere falloff.
	// Shared by the flock ambience and every per-bird caw.
	USoundAttenuation* AttenuationAsset = FlockSoundAttenuationOverride.LoadSynchronous();
	FSoundAttenuationSettings Attenuation;
	const bool bUseAttenuationOverride = (AttenuationAsset == nullptr);
	if (bUseAttenuationOverride)
	{
		// The cue may have no attenuation of its own; build a sphere falloff from
		// the inner/outer radius settings so the sound does not stay global.
		Attenuation.bAttenuate = true;
		Attenuation.bSpatialize = true;
		Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Attenuation.FalloffMode = ENaturalSoundFalloffMode::Silent;
		Attenuation.dBAttenuationAtMax = -60.f;
		const float Inner = FMath::Max(FlockSoundInnerRadius, 0.f);
		const float Outer = FMath::Max(FlockSoundOuterRadius, Inner + 1.f);
		Attenuation.AttenuationShapeExtents = FVector(Inner, 0.f, 0.f);
		Attenuation.FalloffDistance = Outer - Inner;
	}

	// Per-bird caws decide the flock size: array length = bird count.
	const bool bUsePerBirdSounds = BirdSounds.Num() > 0;
	const int32 NumBirdsToSpawn = bUsePerBirdSounds ? BirdSounds.Num() : BirdCount;

	for (int32 i = 0; i < NumBirdsToSpawn; ++i)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AFlockBird* Bird = World->SpawnActor<AFlockBird>(AFlockBird::StaticClass(), GetActorTransform(), Params);
		if (!Bird)
		{
			continue;
		}

		Bird->MeshComponent->SetSkeletalMesh(Mesh);

		if (AnimClassToUse)
		{
			Bird->MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Bird->MeshComponent->SetAnimInstanceClass(AnimClassToUse);
		}
		else
		{
			UAnimationAsset* Anim = MainAnim;
			if (AltAnim && (Rng.FRand() < AltAnimChance || !MainAnim))
			{
				Anim = AltAnim;
			}
			if (Anim)
			{
				Bird->MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				Bird->MeshComponent->SetAnimation(Anim);
				Bird->MeshComponent->SetPlayRate(Rng.FRandRange(0.95f, 1.1f));
				Bird->MeshComponent->Play(true);
			}
		}

		Bird->OwnerPath = this;
		Bird->DistanceAlongSpline = SplineLength * Rng.FRand();
		Bird->Speed = Rng.FRandRange(FMath::Min(BirdSpeedMin, BirdSpeedMax), FMath::Max(BirdSpeedMin, BirdSpeedMax));

		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const float Radius = Rng.FRandRange(0.f, BirdOffsetRadius);
		Bird->LateralOffset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
			Rng.FRandRange(-0.3f, 0.3f) * Radius);

		Bird->BobAmplitude = BirdBobAmplitude * Rng.FRandRange(0.6f, 1.4f);
		Bird->BobFrequency = BirdBobFrequency * Rng.FRandRange(0.8f, 1.3f);
		Bird->BobPhase = Rng.FRand();
		Bird->YawOffsetDeg = BirdYawOffsetDeg;

		Bird->FlightDuration = FlightDuration;
		Bird->DepartDelay = Rng.FRandRange(0.f, FMath::Max(DepartScatterDelay, 0.f));
		Bird->DepartSpeedMultiplier = DepartSpeedMultiplier;
		Bird->DepartYawJitterDeg = Rng.FRandRange(-15.f, 15.f);

		if (bUsePerBirdSounds)
		{
			if (USoundBase* Caw = BirdSounds[i].LoadSynchronous())
			{
				Bird->SetupCaw(Caw, FlockSoundVolume, AttenuationAsset, Attenuation, bUseAttenuationOverride,
					BirdCawIntervalMin, BirdCawIntervalMax);
			}
			else
			{
				UE_LOG(LogBirdFlock, Warning,
					TEXT("BirdFlightPath '%s': BirdSounds[%d] failed to load, this bird is silent."),
					*GetName(), i);
			}
		}

		const float Scale = Rng.FRandRange(
			FMath::Min(BirdScaleRange.X, BirdScaleRange.Y),
			FMath::Max(BirdScaleRange.X, BirdScaleRange.Y));
		Bird->SetActorScale3D(FVector(Scale));

		ActiveBirds.Add(Bird);
	}

	// Optional shared ambience on the actor itself; per-bird caws are attached to birds.
	if (USoundBase* SharedSound = FlockSound.LoadSynchronous())
	{
		FlockAudio->SetSound(SharedSound);
		FlockAudio->SetVolumeMultiplier(FlockSoundVolume);

		if (bUseAttenuationOverride)
		{
			FlockAudio->AttenuationSettings = nullptr;
			FlockAudio->bOverrideAttenuation = true;
			FlockAudio->AttenuationOverrides = Attenuation;
		}
		else
		{
			FlockAudio->bOverrideAttenuation = false;
			FlockAudio->AttenuationSettings = AttenuationAsset;
		}

		FlockAudio->Play();
	}
	else if (BirdSounds.Num() == 0)
	{
		UE_LOG(LogBirdFlock, Warning,
			TEXT("BirdFlightPath '%s': no FlockSound and no BirdSounds, flock will be silent. / Ni zvuka stai, ni krikov ptits ne zadano."),
			*GetName());
	}

	FlockState = EFlockState::Active;

	UE_LOG(LogBirdFlock, Log, TEXT("BirdFlightPath '%s': flock activated with %d bird(s)."),
		*GetName(), ActiveBirds.Num());

	if (bTriggerOnlyOnce && !bOneShotConsumed)
	{
		bOneShotConsumed = true;
		OnOneShotFlockTriggered();
	}
}

void ABirdFlightPath::DeactivateFlock(bool bFlewAway)
{
	UE_LOG(LogBirdFlock, Log, TEXT("BirdFlightPath '%s': flock deactivated (%s), player at %.0f cm."),
		*GetName(),
		bFlewAway ? TEXT("birds flew away") : TEXT("player left the radius"),
		GetPlayerDistance());

	for (TObjectPtr<AFlockBird>& Bird : ActiveBirds)
	{
		if (Bird)
		{
			Bird->Destroy();
		}
	}
	ActiveBirds.Reset();

	if (FlockAudio && FlockAudio->IsPlaying())
	{
		FlockAudio->Stop();
	}

	FlockState = (bTriggerOnlyOnce || !bRetriggerAfterDeactivate) ? EFlockState::DoneForever : EFlockState::Idle;
	RetryCountdown = 0.f;
}

void ABirdFlightPath::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateFlock();
	Super::EndPlay(EndPlayReason);
}

void ABirdFlightPath::ApplyCircleSplinePreset()
{
	ApplySplinePreset(true);
}

void ABirdFlightPath::ApplyLineSplinePreset()
{
	ApplySplinePreset(false);
}

void ABirdFlightPath::ApplySplinePreset(bool bCircle)
{
	Modify();
	FlightSpline->Modify();
	FlightSpline->ClearSplinePoints(false);

	if (bCircle)
	{
		constexpr int32 NumPoints = 12;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float Angle = static_cast<float>(i) / NumPoints * 2.f * PI;
			FlightSpline->AddSplinePoint(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SplinePresetRadius,
				ESplineCoordinateSpace::Local, false);
		}
		FlightSpline->SetClosedLoop(true, false);
	}
	else
	{
		FlightSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		FlightSpline->AddSplinePoint(FVector(SplinePresetRadius * 4.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
		FlightSpline->SetClosedLoop(false, false);
	}

	FlightSpline->UpdateSpline();
}
