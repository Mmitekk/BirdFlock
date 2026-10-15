#include "FlockBird.h"
#include "BirdFlightPath.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

namespace
{
	// A bird this far from the path actor is considered out of sight and stops existing.
	constexpr float DepartDespawnDistance = 12000.f;
	// Departure climb: upward component added to the outward horizontal direction.
	constexpr float DepartClimb = 0.22f;
	// Safety cap so a broken direction can never keep a bird alive forever.
	constexpr float DepartSafetySeconds = 30.f;
	// Floor for the random caw replay interval.
	constexpr float CawReplayMinInterval = 0.5f;
}

AFlockBird::AFlockBird()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BirdMesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->bReceivesDecals = false;
	MeshComponent->SetEnableGravity(false);
}

void AFlockBird::SetupCaw(USoundBase* Sound, float VolumeMultiplier,
	USoundAttenuation* AttenuationAsset,
	const FSoundAttenuationSettings& AttenuationOverride, bool bUseAttenuationOverride,
	float ReplayIntervalMin, float ReplayIntervalMax)
{
	if (!Sound)
	{
		return;
	}

	CawAudio = NewObject<UAudioComponent>(this, TEXT("CawAudio"));
	CawAudio->bAutoActivate = false;
	CawAudio->SetIsReplicated(false);
	CawAudio->RegisterComponent();
	CawAudio->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	CawAudio->SetSound(Sound);
	CawAudio->SetVolumeMultiplier(VolumeMultiplier);

	if (bUseAttenuationOverride)
	{
		CawAudio->AttenuationSettings = nullptr;
		CawAudio->bOverrideAttenuation = true;
		CawAudio->AttenuationOverrides = AttenuationOverride;
	}
	else if (AttenuationAsset)
	{
		CawAudio->bOverrideAttenuation = false;
		CawAudio->AttenuationSettings = AttenuationAsset;
	}

	CawAudio->Play();

	const float Min = FMath::Min(ReplayIntervalMin, ReplayIntervalMax);
	const float Max = FMath::Max(ReplayIntervalMin, ReplayIntervalMax);
	const float Interval = FMath::Max(FMath::FRandRange(Min, Max), CawReplayMinInterval);
	GetWorldTimerManager().SetTimer(CawTimerHandle, this, &AFlockBird::PlayCaw, Interval, true, Interval);
}

void AFlockBird::PlayCaw()
{
	if (bFinished || !CawAudio)
	{
		return;
	}
	CawAudio->Play();
}

void AFlockBird::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CawTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AFlockBird::StartDeparture()
{
	const ABirdFlightPath* Path = OwnerPath.Get();

	// Fly away from the spline center, with a per-bird yaw scatter and a slight climb.
	FVector Outward(1.f, 0.f, 0.f);
	if (Path)
	{
		const FVector FromPath = GetActorLocation() - Path->GetActorLocation();
		const FVector Flat(FromPath.X, FromPath.Y, 0.f);
		if (Flat.SizeSquared() > 1.f)
		{
			Outward = Flat.GetSafeNormal();
		}
	}
	Outward = Outward.RotateAngleAxis(DepartYawJitterDeg, FVector::UpVector);
	DepartDirection = (Outward + FVector(0.f, 0.f, DepartClimb)).GetSafeNormal();
	bDeparting = true;
}

void AFlockBird::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ABirdFlightPath* Path = OwnerPath.Get();
	if (!Path || bFinished)
	{
		return;
	}

	TimeAlive += DeltaSeconds;
	const float Bob = FMath::Sin((TimeAlive * BobFrequency + BobPhase) * 2.f * PI) * BobAmplitude;

	// ------- Departure: straight line away from the spline until out of sight. / Otletpryamoi liniei ot splaina do skrytiya iz vidu.
	if (bDeparting)
	{
		DepartTime += DeltaSeconds;
		const float DepartSpeed = Speed * FMath::Max(DepartSpeedMultiplier, 0.1f);
		SetActorLocation(GetActorLocation() + DepartDirection * (DepartSpeed * DeltaSeconds) + FVector(0.f, 0.f, Bob));

		if (!DepartDirection.IsNearlyZero())
		{
			const float Yaw = DepartDirection.Rotation().Yaw + YawOffsetDeg;
			SetActorRotation(FRotator(0.f, Yaw, 0.f));
		}

		if (DepartTime > DepartSafetySeconds ||
			FVector::Dist(GetActorLocation(), Path->GetActorLocation()) > DepartDespawnDistance)
		{
			bFinished = true;
			MeshComponent->SetVisibility(false, true);
			SetActorTickEnabled(false);
		}
		return;
	}

	// ------- Self-managed departure timer while still following the spline.
	if (FlightDuration > 0.f)
	{
		FlightTime += DeltaSeconds;
		if (FlightTime >= FlightDuration + DepartDelay)
		{
			StartDeparture();
		}
	}

	const USplineComponent* Spline = Path->FlightSpline;
	if (!Spline)
	{
		return;
	}

	const float SplineLength = Spline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	DistanceAlongSpline += Speed * DeltaSeconds * (bPingPongReverse ? -1.f : 1.f);

	switch (Path->FlightMode)
	{
	case EBirdFlightMode::Loop:
		// Spline component is flagged closed-loop for this mode, so wrapping the
		// distance flies the straight return segment from end back to start.
		DistanceAlongSpline = FMath::Fmod(DistanceAlongSpline + SplineLength, SplineLength);
		break;

	case EBirdFlightMode::PingPong:
		if (DistanceAlongSpline >= SplineLength)
		{
			DistanceAlongSpline = SplineLength - (DistanceAlongSpline - SplineLength);
			bPingPongReverse = true;
		}
		else if (DistanceAlongSpline <= 0.f)
		{
			DistanceAlongSpline = -DistanceAlongSpline;
			bPingPongReverse = false;
		}
		break;

	case EBirdFlightMode::SinglePass:
		if (DistanceAlongSpline >= SplineLength)
		{
			DistanceAlongSpline = SplineLength;
			bFinished = true;
			MeshComponent->SetVisibility(false, true);
			SetActorTickEnabled(false);
			return;
		}
		break;
	}

	const FVector BaseLocation = Spline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
	const FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World) *
		(bPingPongReverse ? -1.f : 1.f);

	SetActorLocation(BaseLocation + LateralOffset + FVector(0.f, 0.f, Bob));

	if (!Direction.IsNearlyZero())
	{
		const float Yaw = Direction.Rotation().Yaw + YawOffsetDeg;
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
}
