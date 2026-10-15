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

	/** Birds fly the spline from start to end, then depart along their movement direction and vanish.
	 * Птицы пролетают от начала до конца сплайна, затем улетают прочь по направлению движения и исчезают. */
	SinglePass UMETA(DisplayName = "Single Pass (один пролёт)"),

	/** Birds fly start -> end -> back to start, then depart along their heading.
	 * Птицы летят от начала до конца и обратно, после возврата улетают по курсу. */
	PingPong UMETA(DisplayName = "Ping-Pong (туда-обратно)")
};
