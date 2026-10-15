# BirdFlock

**Unreal Engine 5.2–5.8 plugin: Long Dark style flying bird flocks.**
**Плагин для Unreal Engine 5.2–5.8: стаи летающих птиц в стиле The Long Dark.**

**[🇷🇺 Русский](#russian)** | **[🇬🇧 English](#english)**

---

<a id="russian"></a>
## Русский

> **⚠️ Ассеты птиц НЕ поставляются с плагином.** Меш, полётную анимацию и звуки нужно взять
> отдельно — например, пак птиц/животных на Fab (Unreal Marketplace) или любые свои ассеты.

**BirdFlock** — плагин, добавляющий стаи летающих птиц в стиле The Long Dark.
Разместите на уровне блюпринт **BirdFlightPath**, отредактируйте его сплайн — при приближении игрока
появляется стая (с настраиваемой вероятностью), летит по траектории, кричит, а затем либо улетает
из виду, либо выгружается по дистанции.

## Возможности

- **Траектории по сплайнам** — точки редактируются во вьюпорте; кнопки-пресеты **Circle** и **Line**.
- **Три режима полёта** — *Loop* (бесконечное кружение, разомкнутая траектория замыкается прямой), *Single Pass* (один пролёт и повтор через паузу) и *Ping-Pong* (туда-обратно).
- **Триггер приближения** — кольцо дистанций [мин..макс], вероятность появления и автоматические повторные проверки, пока игрок в зоне.
- **Прилёт издали** — птицы не появляются на сплайне, а прилетают с настраиваемого расстояния от игрока, из-за пределов видимости.
- **Реалистичный отлёт** — через заданное время полёта птицы по одной срываются, улетают из виду, и только потом стая выгружается; разлетаются кто куда или уходят вместе в одну сторону (от игрока).
- **Слоты звука на каждую птицу** — массив cue'ов: длина массива = количество птиц, у каждой свой приаттаченный звук со случайными повторами; плюс опциональный общий фоновый loop.
- **Единоразовый режим для сейвов** — галочка и BP-событие, которое стреляет в момент фактического появления стаи; сохранение реализуете сами.
- **Полная настройка птиц** — меш, одиночная анимация (+ вторая «планирующая»), замена на AnimBP, рандомизация масштаба/скорости/смещений, поправка разворота меша.
- **Экономичность** — актор опрашивает игрока по таймеру; птицы тикают, только пока стая жива.
- **Отладка** — сферы радиусов и линия расстояния в PIE; лог-категория `LogBirdFlock`.
- **Кнопка Birds** в полосе категорий панели Details (регистрируется редакторным модулем).

## Установка

1. Скопируйте папку `BirdFlock` в `Plugins/` вашего проекта.
2. C++ проект соберёт плагин сам. Для BP-only проекта соберите один раз:
   ```
   <Engine>\Engine\Build\BatchFiles\Build.bat <Проект>Editor Win64 Development -project="<Путь>\<Проект>.uproject"
   ```
3. Откройте редактор — модули `BirdFlock` и `BirdFlockEditor` подхватятся автоматически.

Совместимость: **UE 5.2 – 5.8** (сборка проверена на 5.2 и 5.8), Windows.

## Быстрый старт

1. В Content Browser: **правый клик → Blueprint Class → All Classes → BirdFlightPath**. Назовите, например, `BP_BirdFlightPath`.
2. Откройте и задайте дефолты: **Bird Mesh**, **Bird Anim to Play** (любой `AnimSequence` играется одним узлом, AnimBP не нужен), опционально **Bird Anim to Play Alt** и звуки (см. *Звуковая модель* ниже).
3. Если меш смотрит не по +X — выставьте **Bird Yaw Offset Deg** (у большинства паков помогает -90).
4. Перетащите блюпринт на уровень. Выделите сплайн и правьте точки вручную или нажмите **Spline Preset → Apply Circle Spline Preset** (радиус = `Spline Preset Radius`).
5. Запустите PIE: войдите в зону триггера — появится стая; уйдите за красную сферу — выгрузится.

### Опционально: скрипт автоматизации

Тот же блюпринт можно создать питоном (в редакторе):

```python
import unreal

factory = unreal.BlueprintFactory()
factory.set_editor_property("ParentClass", unreal.BirdFlightPath)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "BP_BirdFlightPath", "/Game/Birds", unreal.Blueprint, factory)

cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("BirdMesh", unreal.load_object(None, "/Game/Birds/SK_MyBird.SK_MyBird"))
cdo.set_editor_property("BirdAnimToPlay", unreal.load_object(None, "/Game/Birds/ANIM_MyBird_Fly.ANIM_MyBird_Fly"))

unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

Внимание: питон 5.2 не умеет присваивать `TSoftObjectPtr` из `SoftObjectPath` — передавайте загруженный объект (сериализуется всё равно как мягкая ссылка).

## Настройки

### Birds|Flock (Стая)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Bird Count` | 5 | Сколько птиц в стае. Игнорируется, если `Bird Sounds` не пуст |
| `Bird Mesh` | — | Скелетная сетка одной птицы |
| `Bird Anim to Play` | — | Анимация полёта одним узлом |
| `Bird Anim to Play Alt` | — | Вторая анимация; ~35% птиц летают с ней вместо основной, для разнообразия |
| `Bird Anim Class` | — | AnimBlueprint; если задан — заменяет одиночные анимации |
| `Bird Scale Range` | 0.9..1.15 | Случайный масштаб каждой птицы |
| `Bird Speed Min/Max` | 220 / 320 | Скорость полёта по сплайну, см/с |
| `Bird Offset Radius` | 150 | Максимальное случайное смещение от точки сплайна, см (разброс стаи) |
| `Bird Bob Amplitude/Frequency` | 25 / 1.2 | Покачивание по вертикали, см и Гц |
| `Bird Yaw Offset Deg` | 0 | Доворот по курсу; лечит меши, свёрстанные боком или хвостом вперёд |

### Birds|Trigger (Триггер)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Trigger Distance Min` | 1500 | Минимальное расстояние срабатывания, см; внутри этого радиуса проверки не идут |
| `Trigger Distance Max` | 7000 | Максимальное расстояние, см; вход в радиус запускает проверки |
| `Trigger Probability` | 0.65 | Вероятность (0..1) за попытку, пока игрок в зоне [мин..макс] |
| `Trigger Retry Interval` | 6 | Секунды между повторными проверками в зоне; 0 = одна попытка за заход |
| `Retrigger After Deactivate` | true | Может ли стая сработать снова после выгрузки (игнорируется в one-shot) |
| `Trigger Only Once` | false | Единоразовый режим — см. *Единоразовые ивенты и сохранение* |

### Birds|Deactivate (Выгрузка)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Deactivate Distance` | 9000 | Игрок дальше этого — птицы выгружаются. Фактически не меньше `Trigger Distance Max` + 5 м |

### Birds|Flight (Полёт)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Flight Mode` | Loop | Loop / Single Pass / Ping-Pong |
| `Approach From Distance` | вкл | Птицы прилетают издалека (кольцо вокруг игрока), а не появляются на сплайне |
| `Approach Distance` | 12000 | С какого расстояния от игрока начинается подлёт, см (±15% разброс) |
| `Flight Duration` | 45 | Секунд на сплайне, после чего птицы срываются и визит заканчивается. 0 = бесконечно |
| `Depart Scatter Delay` | 2.5 | Птицы срываются по очереди в течение этого случайного окна, сек |
| `Depart Speed Multiplier` | 1.8 | Разгон при отлёте: улетают быстрее, чем летают по маршруту |
| `Depart Anim Rate Multiplier` | 1.5 | Скорость взмахов крыльев (анимации) при отлёте |
| `Depart Bob Multiplier` | 1.0 | Амплитуда покачивания при отлёте; 0 — летят по прямой без качания |
| `Depart In One Direction` | выкл | При отлёте стая улетает вместе в одну сторону (от игрока) вместо разлёта кто куда |
| `Single Pass Respawn Delay` | 8 | Пауза до повторного пролёта (Single Pass без отлёта) |
| `Spline Preset Radius` | 800 | Радиус для кнопки-пресета окружности |
| `Random Seed` | 0 | 0 = случайно каждый раз; число = воспроизводимая расстановка и броски |

### Birds|Performance (Производительность)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Player Check Interval` | 10 | Как часто (сек) актор проверяет игрока: попытки триггера и проверка выгрузки. Больше = дешевле; срабатывание и выгрузка могут запаздывать на это время |
| `Draw Debug Distances` | false | Только PIE: зелёная сфера = Trigger Distance Max, оранжевая = Trigger Distance Min, красная = радиус выгрузки, жёлтая линия к игроку с текущим расстоянием |

### Birds|Sound (Звук)

| Параметр | Дефолт | Описание |
|---|---|---|
| `Bird Sounds` | пусто | **Крики по одному на птицу.** Если массив не пуст — количество птиц = количеству файлов (`Bird Count` игнорируется), у каждой птицы свой приаттаченный звук |
| `Bird Caw Interval Min/Max` | 4 / 10 | Секунды между повторами крика у каждой птицы (случайно для каждой) |
| `Flock Sound` | — | Опциональный общий фоновый loop на акторе; наслаивается поверх криков |
| `Flock Sound Volume` | 1.0 | Общая громкость (и фона, и криков) |
| `Flock Sound InnerRadius/OuterRadius` | 1500 / 6000 | Полная громкость внутри / тишина за, см — применяется и к крикам |
| `Flock Sound Attenuation Override` | — | Ассет затухания вместо радиусов |

## Звуковая модель

Если `Bird Sounds` заполнен, каждая птица получает собственный `AudioComponent`, **приаттаченный к ней**:
звук движется и затухает вместе с птицей, первый крик звучит сразу при появлении, дальше каждая ворона
повторяет свой cue через случайный интервал из `Bird Caw Interval Min..Max`. Длина массива задаёт
размер стаи, поэтому каждой птице можно указать отдельный файл. Если у cue нет своего затухания,
плагин строит сферу (`Natural Sound`, тишина за внешним радиусом) из
`Flock Sound InnerRadius/OuterRadius`; ассет затухания в слое override заменяет это.

## Единоразовые ивенты и сохранение

`Trigger Only Once` — стая появляется **один раз за сессию** (полный визит: полёт + отлёт), после чего
актор больше не срабатывает; `Retrigger After Deactivate` игнорируется. В момент **фактического**
появления (бросок вероятности удался) ровно один раз стреляет BP-событие
**`OnOneShotFlockTriggered`** — переопределите его в блюпринте и запишите событие в свой SaveObject.
С выключенной галочкой ивент не срабатывает никогда, стая периодическая.

Сохранение между сессиями намеренно оставлено вам: в `BeginPlay` проверьте свой SaveObject на запись
об этой стае (дайте каждому размещённому актору узнаваемое имя/позицию) и удалите актор, если стая
здесь уже была.

## Свои ассеты птиц

Меши, анимации и звуки птиц не распространяются с плагином — лицензии маркетплейсов это запрещают,
а плагин должен оставаться универсальным. Подойдёт любой SkeletalMesh птицы: назначьте его вместе с
полётным `AnimSequence` (проигрывание одним узлом) или AnimBP.

Если хотите упаковать ассеты **внутрь плагина для локального использования** — смигрируйте их в
редакторе (правый клик по ассету → **Asset Actions → Migrate** → выберите папку внутри
`Plugins/BirdFlock/Content`), затем укажите дефолты блюпринта на пути `/BirdFlock/...`.
Плагин включает поддержку контента (`CanContainContent`), но репозиторий игнорирует `Content/` —
смегрированные ассеты останутся локальными и не попадут в публичный репозиторий.

## Технические заметки

- Птицы спавнятся без коллизии и никогда не блокируют трейсы игрока.
- Отлёт считается завершённым, когда птица дальше ~120 м от актора (страховка 30 с).
- Радиус выгрузки фактически равен `max(Deactivate Distance, Trigger Distance Max + 5 м)`, чтобы стая не выгружалась, пока игрок в зоне триггера.
- События активации/выгрузки пишутся в лог `LogBirdFlock`.
- Все тайминги — константами в начале `Source/BirdFlock/Private/FlockBird.cpp`.

## Лицензия

[MIT](LICENSE) — используйте свободно в своих проектах.

---

<a id="english"></a>
## English

> **⚠️ Bird assets are NOT shipped with this plugin.** The mesh, flight animation and sounds are
> yours to provide — grab a bird/animal pack on Fab (Unreal Marketplace) separately, or use your
> own assets.

**BirdFlock** adds Long Dark style flying bird flocks.
Place a **BirdFlightPath** blueprint on the level, edit its spline — and when the player comes
close, a flock spawns (with a configurable probability), flies along the trajectory, caws, then
either flies away out of sight or despawns by distance.

## Features

- **Spline flight paths** — edit points in the viewport; **Circle** and **Line** preset buttons.
- **Three flight modes** — *Loop* (endless circling, open splines are closed by a straight return leg), *Single Pass* (fly through, respawn after a delay) and *Ping-Pong*.
- **Proximity trigger** — a [min..max] distance band, appearance probability and automatic retries while the player stays in the band.
- **Fly-in from afar** — birds don't pop up on the spline; they arrive from a configurable distance away, beyond the player's sight.
- **Realistic departure** — after a configurable flight duration the birds scatter away one by one, fly out of sight, and only then the flock despawns; they either scatter in all directions or leave together in one direction (away from the player).
- **Per-bird sound slots** — an array of cues: array length = bird count, every bird carries its own attached audio replaying at random intervals; optional shared ambience loop.
- **One-shot mode for save systems** — a checkbox plus a BlueprintImplementableEvent fired when the one-shot flock actually appears, so you can persist it yourself.
- **Fully configurable birds** — mesh, single-node animation (+ optional second "glide" animation), AnimBP override, scale/speed/offset randomization, yaw correction for meshes authored off-axis.
- **Performance-friendly** — the actor polls the player on a configurable timer; birds tick only while the flock is alive.
- **Debug tools** — trigger/deactivate radius spheres and a live distance line in PIE; `LogBirdFlock` log category.
- **Details panel shortcut** — an editor module registers a **Birds** button in the Details category strip.

## Installation

1. Copy the `BirdFlock` folder into your project's `Plugins/` directory.
2. For C++ projects the plugin builds with the project. For Blueprint-only projects build it once:
   ```
   <Engine>\Engine\Build\BatchFiles\Build.bat <YourProject>Editor Win64 Development -project="<Path>\<YourProject>.uproject"
   ```
3. Open the editor — the `BirdFlock` and `BirdFlockEditor` modules load automatically.

Compatibility: **UE 5.2 – 5.8** (builds verified on 5.2 and 5.8), Windows.

## Quick start

1. In the Content Browser: **right click → Blueprint Class → All Classes → BirdFlightPath**. Name it e.g. `BP_BirdFlightPath`.
2. Open it and set the defaults: **Bird Mesh**, **Bird Anim to Play** (any `AnimSequence` is played as a single node, no AnimBP required), optionally **Bird Anim to Play Alt**, and sounds (see *Sound model* below).
3. If your mesh does not face +X, set **Bird Yaw Offset Deg** (-90 fixes most packs).
4. Drag the blueprint onto the level. Select the spline and either edit points manually or press **Spline Preset → Apply Circle Spline Preset** (radius = `Spline Preset Radius`).
5. Play in editor: walk into the trigger band and the flock appears; walk away beyond the red debug sphere and it despawns.

### Optional: automation script

The same BP can be created from Python (editor session):

```python
import unreal

factory = unreal.BlueprintFactory()
factory.set_editor_property("ParentClass", unreal.BirdFlightPath)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "BP_BirdFlightPath", "/Game/Birds", unreal.Blueprint, factory)

cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("BirdMesh", unreal.load_object(None, "/Game/Birds/SK_MyBird.SK_MyBird"))
cdo.set_editor_property("BirdAnimToPlay", unreal.load_object(None, "/Game/Birds/ANIM_MyBird_Fly.ANIM_MyBird_Fly"))

unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

Note: UE 5.2 Python cannot assign `TSoftObjectPtr` from a `SoftObjectPath` — pass a loaded object instead (it still serializes as a soft reference).

## Settings

### Birds|Flock

| Property | Default | Description |
|---|---|---|
| `Bird Count` | 5 | How many birds spawn. Ignored when `Bird Sounds` is not empty |
| `Bird Mesh` | None | Skeletal mesh of one bird |
| `Bird Anim to Play` | None | Flight animation played as a single node |
| `Bird Anim to Play Alt` | None | Optional second animation; ~35% of birds play it instead of the main one, for variety |
| `Bird Anim Class` | None | Optional AnimBlueprint; when set it replaces the single-node animations |
| `Bird Scale Range` | 0.9..1.15 | Random per-bird scale range |
| `Bird Speed Min/Max` | 220 / 320 | Flight speed along the spline, cm/s |
| `Bird Offset Radius` | 150 | Max random offset from the spline point, cm (flock spread) |
| `Bird Bob Amplitude/Frequency` | 25 / 1.2 | Vertical bobbing, cm and Hz |
| `Bird Yaw Offset Deg` | 0 | Extra yaw on top of flight direction; fixes meshes authored sideways/backwards |

### Birds|Trigger

| Property | Default | Description |
|---|---|---|
| `Trigger Distance Min` | 1500 | Closest distance at which the trigger can fire, cm. Inside this radius no attempts are made |
| `Trigger Distance Max` | 7000 | Farthest distance, cm. Entering this radius starts the attempts |
| `Trigger Probability` | 0.65 | Chance (0..1) per attempt while the player is inside [Min..Max] |
| `Trigger Retry Interval` | 6 | Seconds between retries while the player stays in the band; 0 = single attempt per approach |
| `Retrigger After Deactivate` | true | Allow the flock to trigger again on the next approach (ignored when one-shot is on) |
| `Trigger Only Once` | false | One-shot mode — see *One-shot events & saving* |

### Birds|Deactivate

| Property | Default | Description |
|---|---|---|
| `Deactivate Distance` | 9000 | Player farther than this → birds despawn. Effectively clamped to at least `Trigger Distance Max` + 5 m |

### Birds|Flight

| Property | Default | Description |
|---|---|---|
| `Flight Mode` | Loop | Loop / Single Pass / Ping-Pong |
| `Approach From Distance` | On | Birds fly in from a ring around the player instead of popping up on the spline |
| `Approach Distance` | 12000 | How far from the player the fly-in starts, cm (±15% spread) |
| `Flight Duration` | 45 | Seconds on the spline before the birds scatter away and the visit ends. 0 = fly forever |
| `Depart Scatter Delay` | 2.5 | Departing birds join the departure within this random window, seconds |
| `Depart Speed Multiplier` | 1.8 | Speed multiplier while departing — they flee faster than they cruise |
| `Depart Anim Rate Multiplier` | 1.5 | Wing flap (animation) speed while departing |
| `Depart Bob Multiplier` | 1.0 | Bobbing amplitude while departing; 0 — straight flight, no bobbing |
| `Depart In One Direction` | Off | On departure the flock leaves together in one direction (away from the player) instead of scattering |
| `Single Pass Respawn Delay` | 8 | Seconds before birds respawn after a completed pass (Single Pass without departure) |
| `Spline Preset Radius` | 800 | Radius used by the circle preset button |
| `Random Seed` | 0 | 0 = random each time; any other value = reproducible layout and rolls |

### Birds|Performance

| Property | Default | Description |
|---|---|---|
| `Player Check Interval` | 10 | How often, seconds, the actor checks the player (trigger attempts and despawn check). Bigger = cheaper; trigger and despawn can lag by up to this value |
| `Draw Debug Distances` | false | PIE only: green sphere = Trigger Distance Max, orange = Trigger Distance Min, red = despawn radius, yellow line to the player with the live distance |

### Birds|Sound

| Property | Default | Description |
|---|---|---|
| `Bird Sounds` | Empty | **Per-bird caw cues.** When not empty, bird count = number of entries (`Bird Count` is ignored) and every bird carries its own attached audio |
| `Bird Caw Interval Min/Max` | 4 / 10 | Seconds between caw replays per bird (random per bird) |
| `Flock Sound` | None | Optional shared ambience loop on the actor; layers on top of per-bird caws |
| `Flock Sound Volume` | 1.0 | Volume multiplier for both ambience and caws |
| `Flock Sound InnerRadius/OuterRadius` | 1500 / 6000 | Full volume inside / silence beyond, cm — used for caws too |
| `Flock Sound Attenuation Override` | None | Attenuation asset replacing the radii above |

## Sound model

If `Bird Sounds` is filled, each spawned bird gets its own `AudioComponent` **attached to the bird**:
it moves and attenuates with it, the first caw plays immediately, then every bird replays its cue
at its own random interval from `Bird Caw Interval Min..Max`. Because the array length defines the
flock size, you can give every bird a distinct cue. If your cue has no attenuation of its own, the
plugin builds a sphere falloff (`Natural Sound`, silence beyond the outer radius) from
`Flock Sound InnerRadius/OuterRadius`; an attenuation asset in the override slot replaces that.

## One-shot events & saving

`Trigger Only Once` makes the flock appear **once per session** (the full visit: flight + departure),
after which the actor never triggers again — `Retrigger After Deactivate` is ignored. At the moment
the one-shot flock *actually appears* (i.e. the probability roll succeeded) the Blueprint event
**`OnOneShotFlockTriggered`** fires exactly once — override it in your blueprint and write the record
into your save object there. With the checkbox off the event never fires and the flock is periodic.

Persistence across sessions is intentionally yours to implement: in `BeginPlay` check your save
object for a record about this flock (give each placed actor an identifiable name/transform) and
destroy the actor if the flock has already been seen.

## Bring your own bird

No bird meshes, animations or sounds are shipped with the plugin — marketplace and pack licenses
do not allow redistributing them, and the plugin stays universal. Any bird SkeletalMesh
works: assign it with a flight `AnimSequence` (single-node playback) or an AnimBP.

If you want the assets bundled *inside the plugin for local use*, migrate them in the editor
(right click the asset → **Asset Actions → Migrate** → choose a folder under
`Plugins/BirdFlock/Content`), then point your blueprint defaults at `/BirdFlock/...` paths.
The plugin has content support enabled (`CanContainContent`); the repository ignores `Content/`,
so migrated assets stay local and never end up in the public repo.

## Technical notes

- Birds spawn without collision and never block player traces.
- Departure is considered finished when a bird is ~120 m from the actor (30 s safety cap).
- The despawn radius is effectively `max(Deactivate Distance, Trigger Distance Max + 5 m)` so a flock never despawns while the player is still inside the trigger band.
- Activation/deactivation events are logged under the `LogBirdFlock` category.
- All timing constants are at the top of `Source/BirdFlock/Private/FlockBird.cpp`.

## License

[MIT](LICENSE) — use it freely in your projects.
