#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdFlockTypes.h"
#include "FlockBird.generated.h"

class USkeletalMeshComponent;
class USplineComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;
struct FSoundAttenuationSettings;
class ABirdFlightPath;

/**
 * One bird flying along a BirdFlightPath spline. Spawned and configured by ABirdFlightPath,
 * never placed manually.
 * Одна птица, летящая по сплайну BirdFlightPath; порождается и настраивается
 * ABirdFlightPath, вручную не размещается.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class BIRDFLOCK_API AFlockBird : public AActor
{
	GENERATED_BODY()

public:
	AFlockBird();

	/** Bird mesh.
	 * Меш птицы. */
	UPROPERTY(VisibleAnywhere, Category = "Bird")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	/** Per-bird caw audio, attached so it moves and fades with the bird.
	 * Звук крика, приаттаченный к птице: движется и затухает вместе с ней. */
	UPROPERTY(VisibleAnywhere, Category = "Bird")
	TObjectPtr<UAudioComponent> CawAudio;

	/** Path actor this bird belongs to.
	 * Актор-маршрут, которому принадлежит птица. */
	TWeakObjectPtr<const ABirdFlightPath> OwnerPath;

	/** Current progress along the spline, cm.
	 * Текущий прогресс по сплайну, см. */
	float DistanceAlongSpline = 0.f;

	/** Flight speed along the spline, cm/s.
	 * Скорость полёта по сплайну, см/с. */
	float Speed = 250.f;

	/** Fixed world-space offset from the spline point (flock spread).
	 * Фиксированное смещение от точки сплайна (разброс стаи). */
	FVector LateralOffset = FVector::ZeroVector;

	float BobAmplitude = 25.f;
	float BobFrequency = 1.2f;
	float BobPhase = 0.f;

	/** Extra yaw applied on top of flight direction, degrees (fixes mesh forward axis).
	 * Доворот по курсу, градусы (совмещение переда меша). */
	float YawOffsetDeg = 0.f;

	/** Seconds of spline flight before this bird departs on its own (copied from the path config). <=0 = never departs.
	 * Секунды полёта по сплайну до самостоятельного отлёта (из конфига маршрута). <=0 = не улетает. */
	float FlightDuration = 0.f;

	/** Random delay before this bird joins the flock departure, seconds.
	 * Случайная задержка перед отлётом вместе со стаей, секунды. */
	float DepartDelay = 0.f;

	/** Speed multiplier during the departure.
	 * Множитель скорости при отлёте. */
	float DepartSpeedMultiplier = 1.8f;

	/** Random yaw jitter of the departure direction, degrees.
	 * Случайный разброс направления отлёта, градусы. */
	float DepartYawJitterDeg = 0.f;

	/** Ping-pong mode only: currently flying backwards.
	 * Только для ping-pong: сейчас летит назад. */
	bool bPingPongReverse = false;

	/** Bird reached the spline end (Single Pass) or flew away beyond sight (departure).
	 * Птица дошла до конца сплайна (Single Pass) или улетела из виду (отлёт). */
	bool bFinished = false;

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Attach the per-bird caw: plays once immediately, then replays at a random interval.
	 * Пристегнуть крик: играет сразу, потом повторяется по случайному интервалу. */
	void SetupCaw(USoundBase* Sound, float VolumeMultiplier,
		USoundAttenuation* AttenuationAsset,
		const FSoundAttenuationSettings& AttenuationOverride, bool bUseAttenuationOverride,
		float ReplayIntervalMin, float ReplayIntervalMax);

private:
	UFUNCTION()
	void PlayCaw();

	void StartDeparture();

	float TimeAlive = 0.f;
	bool bDeparting = false;
	float FlightTime = 0.f;
	float DepartTime = 0.f;
	FVector DepartDirection = FVector::ZeroVector;
	FTimerHandle CawTimerHandle;
};
