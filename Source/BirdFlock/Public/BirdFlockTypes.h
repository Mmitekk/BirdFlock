#pragma once

#include "CoreMinimal.h"
#include "BirdFlockTypes.generated.h"

/**
 * How birds travel along the flight spline.
 * Как птицы движутся по сплайну.
 */
UENUM(BlueprintType)
enum class EBirdFlightMode : uint8
{
	/** Birds circle along the spline until the departure timer fires. An open spline is closed by a straight return segment.
	 * Птицы кружат по сплайну, пока не наступит время отлёта. Разомкнутая траектория замыкается прямым отрезком от конца к началу. */
	Loop UMETA(DisplayName = "Loop (по кругу)"),

	/** Birds fly the spline once and disappear at the end, then respawn after a delay while the player is still in range.
	 * Птицы пролетают сплайн один раз и исчезают в конце; через паузу появятся снова, пока игрок в зоне. */
	SinglePass UMETA(DisplayName = "Single Pass (один пролёт)"),

	/** Birds fly back and forth along the spline without teleporting to the start.
	 * Птицы летают по сплайну туда-обратно, без телепорта к началу. */
	PingPong UMETA(DisplayName = "Ping-Pong (туда-обратно)")
};
