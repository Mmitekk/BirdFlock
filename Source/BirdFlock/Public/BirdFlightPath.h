#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdFlockTypes.h"
#include "BirdFlightPath.generated.h"

class USplineComponent;
class UAudioComponent;
class USkeletalMesh;
class UAnimationAsset;
class UAnimInstance;
class USoundBase;
class USoundAttenuation;
class AFlockBird;

/**
 * Placeable flight-path actor: edit the spline, and a flock of birds spawns and flies
 * along it when the player comes within the trigger band (with configurable probability),
 * and despawns — or flies away — beyond it.
 *
 * Размещаемый актор-маршрут: отредактируйте сплайн — при приближении игрока к зоне
 * триггера (с заданной вероятностью) появляется стая птиц и летит по траектории,
 * а при удалении выгружается — или улетает прочь.
 */
UCLASS(Blueprintable)
class BIRDFLOCK_API ABirdFlightPath : public AActor
{
	GENERATED_BODY()

public:
	ABirdFlightPath();

	/** Flight trajectory. Edit its points in the viewport.
	 * Траектория полёта; точки редактируются прямо во вьюпорте. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Birds")
	TObjectPtr<USplineComponent> FlightSpline;

	/** Ambient sound of the flock, played while birds are active.
	 * Фоновый звук стаи, играет пока птицы активны. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Birds")
	TObjectPtr<UAudioComponent> FlockAudio;

	// ---------------------------------------------------------------- Flock / Стая

	/** How many birds spawn. Ignored when BirdSounds is not empty.
	 * Сколько птиц в стае. Игнорируется, если массив BirdSounds не пуст. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "1", ClampMax = "64"))
	int32 BirdCount = 5;

	/** Skeletal mesh of one bird (e.g. SK_Crow).
	 * Скелетная сетка одной птицы (например SK_Crow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock")
	TSoftObjectPtr<USkeletalMesh> BirdMesh;

	/** Flight animation played as a single node (e.g. ANIM_Crow_Fly).
	 * Анимация полёта одним узлом (например ANIM_Crow_Fly). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock")
	TSoftObjectPtr<UAnimationAsset> BirdAnimToPlay;

	/** Optional second animation; ~35% of birds play it instead of the main one, for variety (e.g. ANIM_Crow_Glide).
	 * Необязательная вторая анимация; ~35% птиц летают с ней вместо основной, для разнообразия (например ANIM_Crow_Glide). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock")
	TSoftObjectPtr<UAnimationAsset> BirdAnimToPlayAlt;

	/** Optional AnimBlueprint; when set it replaces the single-node animations above.
	 * Необязательный AnimBlueprint; если задан — заменяет одиночные анимации выше. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock")
	TSoftClassPtr<UAnimInstance> BirdAnimClass;

	/** Random per-bird scale range (min..max).
	 * Случайный масштаб каждой птицы (мин..макс). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock")
	FVector2D BirdScaleRange = FVector2D(0.9f, 1.15f);

	/** Min flight speed along the spline, cm/s.
	 * Минимальная скорость полёта по сплайну, см/с. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "1"))
	float BirdSpeedMin = 220.f;

	/** Max flight speed along the spline, cm/s.
	 * Максимальная скорость полёта по сплайну, см/с. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "1"))
	float BirdSpeedMax = 320.f;

	/** Max random offset from the spline point per bird, cm (flock spread).
	 * Максимальное случайное смещение птицы от точки сплайна, см (разброс стаи). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "0"))
	float BirdOffsetRadius = 150.f;

	/** Vertical bobbing amplitude, cm.
	 * Амплитуда покачивания по вертикали, см. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "0"))
	float BirdBobAmplitude = 25.f;

	/** Vertical bobbing frequency, Hz.
	 * Частота покачивания, Гц. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "0"))
	float BirdBobFrequency = 1.2f;

	/** Extra yaw on top of flight direction, degrees. Use if birds fly sideways/backwards with your mesh.
	 * Доворот по курсу поверх направления полёта, градусы. Если птица летит боком или хвостом вперёд — подберите значение. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flock", meta = (ClampMin = "-180", ClampMax = "180"))
	float BirdYawOffsetDeg = 0.f;

	// -------------------------------------------------------------- Trigger / Триггер

	/** Closest distance at which the trigger can fire, cm. Inside this radius no attempts are made.
	 * Минимальное расстояние срабатывания, см; внутри этого радиуса проверки не идут. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger", meta = (ClampMin = "0", UIMin = "0"))
	float TriggerDistanceMin = 1500.f;

	/** Farthest distance at which the trigger can fire, cm. Entering this radius starts the attempts.
	 * Максимальное расстояние срабатывания, см; вход в этот радиус запускает проверки. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger", meta = (ClampMin = "0", UIMin = "0"))
	float TriggerDistanceMax = 7000.f;

	/** Chance (0..1) that the flock appears on each attempt while the player is inside [Min..Max].
	 * Вероятность (0..1) появления стаи при каждой попытке, пока игрок в зоне [мин..макс]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger", meta = (ClampMin = "0", ClampMax = "1"))
	float TriggerProbability = 0.65f;

	/** Seconds between probability retries while the player stays in [Min..Max]. 0 = only the first attempt after entering.
	 * Секунды между повторными проверками, пока игрок в зоне. 0 = только первая попытка при входе. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger", meta = (ClampMin = "0"))
	float TriggerRetryInterval = 6.f;

	/** After despawning, allow the flock to trigger again on the player's next approach. Ignored while bTriggerOnlyOnce is on.
	 * После выгрузки стая может сработать снова при следующем приближении. Игнорируется при включённом bTriggerOnlyOnce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger")
	bool bRetriggerAfterDeactivate = true;

	/** One-shot event: the flock appears only once per session (this very visit), then never again. While the box is checked, OnOneShotFlockTriggered fires on the actual appearance so you can write it into your SaveObject; unchecked, the event never fires and the flock is periodic.
	 * Единоразовый ивент: стая появляется один раз за сессию (этот самый визит), потом никогда. Пока галочка стоит — при фактическом появлении стреляет OnOneShotFlockTriggered для записи в ваш SaveObject; без галочки ивент не срабатывает никогда, стая периодическая. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Trigger")
	bool bTriggerOnlyOnce = false;

	// ----------------------------------------------------------- Deactivate / Выгрузка

	/** Player farther than this (cm) -> birds despawn. Kept at least 5 m beyond TriggerDistanceMax.
	 * Игрок дальше этого расстояния (см) — птицы выгружаются. Не ближе 5 м за TriggerDistanceMax. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Deactivate", meta = (ClampMin = "0", UIMin = "0"))
	float DeactivateDistance = 9000.f;

	// --------------------------------------------------------------- Flight / Полёт

	/** How birds travel along the spline.
	 * Как птицы движутся по сплайну. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight")
	EBirdFlightMode FlightMode = EBirdFlightMode::Loop;

	/** Birds fly in from beyond the player's sight instead of popping up right on the spline.
	 * Птицы прилетают издалека, из-за пределов видимости игрока, а не появляются прямо на сплайне. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight")
	bool bApproachFromDistance = true;

	/** How far from the player the fly-in starts, cm.
	 * С какого расстояния от игрока птицы стартуют полёт к сплайну, см. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "1000", ClampMax = "100000", EditCondition = "bApproachFromDistance"))
	float ApproachDistance = 12000.f;

	/** Seconds the flock keeps flying along the spline before the birds scatter away one by one, fly out of sight, and only then the flock despawns. 0 = fly forever.
	 * Сколько секунд стая летает по сплайну, после чего птицы по одной срываются, улетают из виду, и только затем стая выгружается. 0 = летают бесконечно. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "0"))
	float FlightDuration = 45.f;

	/** Departing birds join the departure within this random window, seconds.
	 * Птицы срываются по очереди в течение этого случайного окна, секунды. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "0", EditCondition = "FlightDuration > 0"))
	float DepartScatterDelay = 2.5f;

	/** Speed multiplier while departing: they flee faster than they cruise.
	 * Множитель скорости при отлёте: улетают быстрее, чем летают по маршруту. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "1", EditCondition = "FlightDuration > 0"))
	float DepartSpeedMultiplier = 1.8f;

	/** Wing flap (animation) speed multiplier while departing.
	 * Множитель скорости взмахов крыльев (анимации) при отлёте. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "0.1", EditCondition = "FlightDuration > 0"))
	float DepartAnimRateMultiplier = 1.5f;

	/** Multiplier of the vertical bobbing amplitude while departing. 0 = straight flight without bobbing.
	 * Множитель амплитуды покачивания при отлёте. 0 — летят по прямой без качания. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "0", EditCondition = "FlightDuration > 0"))
	float DepartBobMultiplier = 1.f;

	/** On departure the flock flies away together in one direction (away from the player) instead of scattering in all directions.
	 * При отлёте стая улетает вместе в одну сторону (от игрока), а не разлетается кто куда. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (EditCondition = "FlightDuration > 0"))
	bool bDepartInOneDirection = false;

	/** Single Pass only: seconds before birds respawn (while the player is still in range).
	 * Только Single Pass: секунды до повторного появления, пока игрок рядом. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "0", EditCondition = "FlightMode == EBirdFlightMode::SinglePass"))
	float SinglePassRespawnDelay = 8.f;

	/** Radius used by the "Spline Preset: Circle" button.
	 * Радиус для кнопки пресета окружности. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight", meta = (ClampMin = "100"))
	float SplinePresetRadius = 800.f;

	/** 0 = fully random each time. Any other value = reproducible bird layout and rolls.
	 * 0 = полностью случайно каждый раз; другое значение — воспроизводимая расстановка и броски. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Flight")
	int32 RandomSeed = 0;

	// ----------------------------------------------------------- Performance / Производительность

	/** How often, in seconds, this actor checks the player: trigger attempts AND the despawn check. Bigger = cheaper; trigger and despawn can lag by up to this value.
	 * Как часто (сек) актор проверяет игрока: попытки триггера и проверка выгрузки. Больше = дешевле; срабатывание и выгрузка могут запаздывать на это время. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Performance", meta = (ClampMin = "0.05", ClampMax = "60"))
	float PlayerCheckInterval = 10.f;

	/** PIE only: green sphere = TriggerDistanceMax, orange = TriggerDistanceMin, red = despawn radius, yellow line to the player with the live distance.
	 * Только в PIE: зелёная сфера = TriggerDistanceMax, оранжевая = TriggerDistanceMin, красная = радиус выгрузки, жёлтая линия к игроку с текущим расстоянием. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Performance")
	bool bDrawDebugDistances = false;

	// ---------------------------------------------------------------- Sound / Звук

	/** Optional shared ambience loop on the actor; layers on top of the per-bird caws. Clear it if you only want local caws.
	 * Необязательный общий фоновый loop на акторе; наслаивается поверх криков отдельных птиц. Очистите, если нужны только локальные крики. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound")
	TSoftObjectPtr<USoundBase> FlockSound;

	/** Volume multiplier of the flock sound (ambience and per-bird caws).
	 * Множитель громкости звука стаи (и фона, и криков птиц). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound", meta = (ClampMin = "0"))
	float FlockSoundVolume = 1.f;

	/** Sound is at full volume inside this radius, cm.
	 * Внутри этого радиуса звук идёт на полную громкость, см. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound", meta = (ClampMin = "0"))
	float FlockSoundInnerRadius = 1500.f;

	/** Sound fades to silence at this distance, cm.
	 * На этом расстоянии звук затухает до тишины, см. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound", meta = (ClampMin = "0"))
	float FlockSoundOuterRadius = 6000.f;

	/** Optional attenuation asset; replaces the inner/outer radius settings above.
	 * Необязательный ассет затухания; заменяет настройки радиусов выше. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound")
	TSoftObjectPtr<USoundAttenuation> FlockSoundAttenuationOverride;

	/** Per-bird caw cues: when not empty, bird count = number of entries (BirdCount is ignored) and every bird carries its own attached audio, replaying at a random BirdCawIntervalMin..Max.
	 * Крики по одному на птицу: если массив не пуст, количество птиц = количеству файлов (BirdCount игнорируется), у каждой птицы свой приаттаченный звук со случайным повтором каждые BirdCawIntervalMin..Макс секунд. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound")
	TArray<TSoftObjectPtr<USoundBase>> BirdSounds;

	/** Min seconds between caw replays per bird.
	 * Минимальный интервал повтора крика у каждой птицы, секунды. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound", meta = (ClampMin = "0.5", ClampMax = "120"))
	float BirdCawIntervalMin = 4.f;

	/** Max seconds between caw replays per bird.
	 * Максимальный интервал повтора крика у каждой птицы, секунды. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Birds|Sound", meta = (ClampMin = "0.5", ClampMax = "120"))
	float BirdCawIntervalMax = 10.f;

	// ---------------------------------------------------- Live birds (debug) / Живые птицы (отладка)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Birds|Debug")
	TArray<TObjectPtr<AFlockBird>> ActiveBirds;

	// -------------------------------------------- Editor buttons / Кнопки редактора

	/** Rebuild the spline as a horizontal circle (Long Dark style circling).
	 * Перестроить сплайн в горизонтальную окружность (кружение как в Long Dark). */
	UFUNCTION(CallInEditor, Category = "Spline Preset")
	void ApplyCircleSplinePreset();

	/** Rebuild the spline as a straight line along the actor's X axis.
	 * Перестроить сплайн в прямую линию вдоль оси X актора. */
	UFUNCTION(CallInEditor, Category = "Spline Preset")
	void ApplyLineSplinePreset();

	/** Fires exactly once, right after a one-shot flock (bTriggerOnlyOnce) actually appears. Hook your save logic here (write the record to your SaveObject yourself); it never fires while bTriggerOnlyOnce is off.
	 * Стреляет ровно один раз, сразу после реального появления одноразовой стаи (bTriggerOnlyOnce). Повесьте сюда логику сохранения (запись в SaveObject делаете сами); при выключенной галочке не срабатывает никогда. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Birds|Trigger")
	void OnOneShotFlockTriggered();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EFlockState : uint8
	{
		Idle,
		Active,
		WaitingRespawn,
		DoneForever
	};

	void ApplySplinePreset(bool bCircle);
	void ActivateFlock();
	void DeactivateFlock(bool bFlewAway = false);
	bool AllBirdsFinished() const;
	float GetPlayerDistance() const;
	float GetEffectiveDeactivateDistance() const;

	EFlockState FlockState = EFlockState::Idle;
	float RetryCountdown = 0.f;
	float RespawnCountdown = 0.f;
	FRandomStream Rng;

	/** Session-only: a one-shot flock already appeared (the BP event has fired).
	 * Только на сессию: одноразовая стая уже появилась (ивент уже стрелял). */
	bool bOneShotConsumed = false;
};
