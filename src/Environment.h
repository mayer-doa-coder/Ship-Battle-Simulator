#pragma once

// Environment build: the world the ship sails through - WHERE it is, what the WEATHER is doing and what TIME it is.
//
// There is no OpenGL in this file, like src/Sky.h and src/Islands.h, so every claim about the atmosphere can be checked on the
// CPU against the very functions the renderer calls.
//
// THE IDEA: three choices, one equation. The player picks a location, a weather and a time of day (the on-screen panel, or the keys
// I, C and T). Each choice is only DATA - a handful of numbers - and composeAtmosphere() turns the three into one Atmosphere: the sun's
// direction and colour, the ambient light, the colours of the horizon and the zenith, the haze, the clouds, the rain. The renderer
// then hands those numbers to the SAME two lights and the SAME shader it always used. Nothing about the illumination model changes
// between noon and a storm at midnight; only the inputs do (that is the answer to "why not a shader per weather", as it was for
// "why not a shader per shading mode").
//
// WHY COMPOSE INSTEAD OF A TABLE OF 75 LOOKS. Five locations x five weathers x three times is 75 pictures. Writing them out by hand
// guarantees that some are inconsistent. Instead each TIME OF DAY supplies a clear-sky base AND an overcast base, each WEATHER supplies
// how overcast it is, how misty, how dark and how much it rains, and the two are blended. A storm at night is therefore the night's
// overcast colours darkened by the storm's factor - not a separately painted scene.
//
// THE DEFAULT (open ocean, sunny, sunset) IS THE GOLDEN HOUR THE SHOWCASE HAS HAD SINCE PHASE 43. Its numbers are copied from
// LightConfig::GOLDEN_HOUR_PROFILE, and "sunny" is built so that every modifier is exactly neutral (a blend of 0, a scale of 1), so the
// program opens on the same picture as the previous build.

#include "Material.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

// ---- the three choices -------------------------------------------------------------------------------------------------

constexpr int LOCATION_COUNT = 5;
constexpr int WEATHER_COUNT = 5;
constexpr int TIME_COUNT = 3;

// The panel and the keys offer one more weather and one more time than there are fixed ones: DYNAMIC. It is stored as the index one past the
// last fixed option, so a choice is still just three small integers (WEATHER_DYNAMIC means "the weather changes by itself", TIME_DYNAMIC "the sun
// moves by itself").
constexpr int WEATHER_DYNAMIC = WEATHER_COUNT;
constexpr int TIME_DYNAMIC = TIME_COUNT;
constexpr int WEATHER_OPTIONS = WEATHER_COUNT + 1;
constexpr int TIME_OPTIONS = TIME_COUNT + 1;

namespace LocationId {
constexpr int OPEN_OCEAN = 0;       // the showcase as it was: distant islands, buoys, sea stacks, a lighthouse
constexpr int JUNGLE_ISLAND = 1;
constexpr int MOUNTAIN_COAST = 2;
constexpr int ROCKY_ISLANDS = 3;
constexpr int FOGGY_COAST = 4;
} // namespace LocationId

namespace WeatherId {
constexpr int SUNNY = 0;
constexpr int CLOUDY = 1;
constexpr int RAINY = 2;
constexpr int MISTY = 3;
constexpr int STORM = 4;
} // namespace WeatherId

namespace TimeId {
constexpr int DAY = 0;
constexpr int SUNSET = 1;
constexpr int NIGHT = 2;
} // namespace TimeId

// Upper case on purpose: the on-screen font (src/Overlay.h) has capitals only.
inline const char* locationName(int i)
{
    static const char* const NAMES[LOCATION_COUNT] = { "OPEN OCEAN", "JUNGLE ISLAND", "MOUNTAIN COAST", "ROCKY ISLANDS", "FOGGY COAST" };
    return NAMES[std::clamp(i, 0, LOCATION_COUNT - 1)];
}
inline const char* weatherName(int i)
{
    static const char* const NAMES[WEATHER_OPTIONS] = { "SUNNY", "CLOUDY", "RAINY", "MISTY", "STORM", "DYNAMIC" };
    return NAMES[std::clamp(i, 0, WEATHER_OPTIONS - 1)];
}
inline const char* timeName(int i)
{
    static const char* const NAMES[TIME_OPTIONS] = { "DAY", "SUNSET", "NIGHT", "DYNAMIC" };
    return NAMES[std::clamp(i, 0, TIME_OPTIONS - 1)];
}

// The camera views (src/Views.h). They live here because the options menu lists them beside the weather and the time of day.
constexpr int CAMERA_MODE_COUNT = 6;
namespace CameraModeId {
constexpr int CHASE = 0;         // the original orbiting third-person camera that follows the ship
constexpr int FREE = 1;          // a detached camera that flies anywhere, above or below the sea
constexpr int FIRST_PERSON = 2;  // standing on the forecastle, looking out over the bow
constexpr int CINEMATIC = 3;     // a film camera that cuts between shots of the ship by itself
constexpr int CAPTAIN = 4;       // through the captain's own eyes at the wheel
constexpr int EXPLORE = 5;         // on foot: the decks and the hold below them (src/Interior.h)
} // namespace CameraModeId

inline const char* cameraModeName(int i)
{
    static const char* const NAMES[CAMERA_MODE_COUNT] = { "CHASE", "FREE", "FIRST PERSON", "CINEMATIC", "CAPTAIN", "BELOW DECK" };
    return NAMES[std::clamp(i, 0, CAMERA_MODE_COUNT - 1)];
}

// The player's current choice. The defaults are the previous build's showcase.
struct EnvironmentChoice {
    int location = LocationId::OPEN_OCEAN;
    int weather = WeatherId::SUNNY;
    int time = TimeId::SUNSET;
    int cameraMode = CameraModeId::CHASE;      // not part of the atmosphere (and last, so {location, weather, time} still initialises it); kept here so the menu can show it selected
};

// Steps a choice one place forward (+1) or back (-1), wrapping round. The key handlers and the panel both use it.
inline int cycleChoice(int value, int count, int direction)
{
    return ((value + direction) % count + count) % count;
}

// ---- the result ----------------------------------------------------------------------------------------------------------

struct Atmosphere {
    // The two lights' inputs. At night the "sun" slot carries the MOON: the same directional light, bluer and dimmer. That is how the
    // project keeps exactly two lights.
    glm::vec3 sunDirection = glm::vec3(-0.65f, -0.30f, -0.55f);   // the way the light TRAVELS (L is its negation)
    glm::vec3 sunColor = glm::vec3(1.15f, 0.80f, 0.50f);
    glm::vec3 ambient = glm::vec3(0.30f, 0.36f, 0.52f);

    // The sky. The haze colour is the horizon colour, always, so the far sea fades into the sky without a seam.
    glm::vec3 horizon = glm::vec3(0.93f, 0.66f, 0.38f);
    glm::vec3 zenith = glm::vec3(0.10f, 0.16f, 0.40f);
    float hazeDensity = 0.03f;
    float mist = 0.0f;         // 0 clear air .. 1 thick fog (the weather's and the place's, the larger): drives the drifting mist particles

    // The sun's (or the moon's) disc: its colour, and how visible it is (cloud and mist hide it).
    glm::vec3 discColor = glm::vec3(1.0f, 0.90f, 0.62f);
    float discVisible = 1.0f;
    float starAlpha = 0.0f;

    // Clouds: how many of the 24 in the table are drawn (a fraction is a fade-in of the last one), how big, what colour, how opaque.
    glm::vec3 cloudColor = glm::vec3(0.98f, 0.80f, 0.62f);
    float cloudCount = 12.0f;
    float cloudScale = 1.0f;
    float cloudAlpha = 1.0f;

    // Weather that moves things.
    float rain = 0.0f;         // 0 none, 1 downpour: how many of the drops are drawn
    float wind = 0.1f;         // 0 calm, 1 gale: leans the rain, sways the trees, flaps the flag harder
    float lightning = 0.0f;    // 1 in a storm: flashes of light on the sky, the sea and the ship

    // The sea.
    float seaRock = 0.012f;    // (no longer used: the ship now rides the real waves, see waveAmp)
    float waveAmp = 0.45f;     // the sea state: how high the waves of src/Waves.h are, as a multiple of the table
    float seaDark = 1.0f;      // multiplies the water's diffuse colour
    float seaShine = 1.0f;     // 1 keeps the ocean's n_s = 160; lower roughens it and weakens the glitter
    float seaSky = 0.0f;       // 0 clear day, 1 overcast or night: the sea then shows more of the sky's own light (its ambient reflectance rises)

    // The ship's lanterns and the gulls.
    float lanterns = 0.0f;     // 0 dark glass, 1 fully lit
    float birds = 1.0f;        // 0 none, 1 a full flock
};

// ---- the data: times of day ------------------------------------------------------------------------------------------

struct TimeBase {
    // Clear sky.
    glm::vec3 sunDirection, sunColor, ambient, horizon, zenith;
    float hazeDensity;
    // The same quantities when the sky is completely overcast.
    glm::vec3 overHorizon, overZenith, overAmbient;
    // A pale fog colour for this time of day (mist pulls the horizon towards it).
    glm::vec3 fog;
    glm::vec3 discColor;
    float stars;
    glm::vec3 cloudColor, overCloudColor;
    float lanterns;
    float birds;
    // How much brighter the water is lit by this time of day: the open ocean's colours are tuned for the golden hour (and for letting the sun's
    // streak dominate), and in full daylight they read as black. 1 leaves them as they were.
    float seaLift;
    // How much of the sky's light the sea gives back at this time of day (the night sea is the colour of the night sky, not black).
    float seaSky;
};

namespace EnvironmentData {

const TimeBase TIMES[TIME_COUNT] = {
    // DAY: a high white sun, a blue sky paling to the horizon.
    { glm::vec3(-0.45f, -0.62f, -0.55f), glm::vec3(1.05f, 1.00f, 0.92f), glm::vec3(0.62f, 0.70f, 0.82f),
      glm::vec3(0.66f, 0.80f, 0.92f),    glm::vec3(0.12f, 0.36f, 0.78f), 0.014f,
      glm::vec3(0.66f, 0.70f, 0.73f),    glm::vec3(0.46f, 0.51f, 0.57f), glm::vec3(0.56f, 0.59f, 0.64f),
      glm::vec3(0.74f, 0.78f, 0.80f),    glm::vec3(1.00f, 0.97f, 0.82f), 0.0f,
      glm::vec3(0.97f, 0.98f, 1.00f),    glm::vec3(0.76f, 0.78f, 0.81f), 0.0f, 1.0f, 2.2f, 0.0f },
    // SUNSET: the golden hour the showcase has had since Phase 43 - the same numbers as LightConfig::GOLDEN_HOUR_PROFILE.
    { glm::vec3(-0.65f, -0.30f, -0.55f), glm::vec3(1.15f, 0.80f, 0.50f), glm::vec3(0.30f, 0.36f, 0.52f),
      glm::vec3(0.93f, 0.66f, 0.38f),    glm::vec3(0.10f, 0.16f, 0.40f), 0.022f,
      glm::vec3(0.62f, 0.46f, 0.40f),    glm::vec3(0.30f, 0.30f, 0.38f), glm::vec3(0.30f, 0.27f, 0.28f),
      glm::vec3(0.80f, 0.62f, 0.50f),    glm::vec3(1.00f, 0.90f, 0.62f), 0.0f,
      glm::vec3(0.98f, 0.80f, 0.62f),    glm::vec3(0.60f, 0.46f, 0.42f), 0.35f, 0.8f, 1.0f, 0.0f },
    // NIGHT: the moon is the directional light - blue, dim - and the ambient is a deep blue.
    { glm::vec3(-0.26f, -0.14f, -0.96f), glm::vec3(0.38f, 0.48f, 0.74f), glm::vec3(0.110f, 0.150f, 0.270f),
      glm::vec3(0.060f, 0.090f, 0.180f), glm::vec3(0.004f, 0.012f, 0.045f), 0.026f,
      glm::vec3(0.040f, 0.048f, 0.075f), glm::vec3(0.012f, 0.016f, 0.030f), glm::vec3(0.075f, 0.085f, 0.130f),
      glm::vec3(0.070f, 0.085f, 0.120f), glm::vec3(0.88f, 0.92f, 1.00f), 1.0f,
      glm::vec3(0.045f, 0.055f, 0.100f),    glm::vec3(0.05f, 0.055f, 0.08f), 1.0f, 0.0f, 1.5f, 0.8f },
};

// ---- the data: weathers ----------------------------------------------------------------------------------------------

struct WeatherMod {
    float overcast;       // 0 clear sky, 1 solid cloud: blends each time's clear colours into its overcast ones
    float darken;         // multiplies the sky colours (a storm sky is far darker than a merely overcast one)
    float sunScale;       // how much of the direct light gets through
    float ambientScale;
    float mist;           // 0 clear air, 1 thick fog: pulls the horizon towards the fog colour and adds haze
    float hazeAdd;        // extra haze density
    float cloudCount, cloudScale, cloudDark, cloudAlpha;
    float rain, wind, lightning;
    float seaRock, seaDark, seaShine;
    float lanternBoost;   // lanterns lit by the dark of the day itself
    float wave;           // the sea state: multiplies every wave of src/Waves.h (a fair day 0.45, a storm 2.0)
};

const WeatherMod WEATHERS[WEATHER_COUNT] = {
    // SUNNY: every modifier neutral. This is what makes the default identical to the previous build.
    { 0.00f, 1.00f, 1.00f, 1.00f, 0.00f, 0.000f, 12.0f, 1.00f, 1.00f, 1.00f, 0.00f, 0.10f, 0.0f, 0.012f, 1.00f, 1.00f, 0.00f, 0.45f },
    // CLOUDY: a grey lid, the sun a weak disc behind it, soft shadowless light.
    { 0.75f, 0.90f, 0.45f, 1.05f, 0.08f, 0.006f, 24.0f, 1.25f, 1.00f, 0.92f, 0.00f, 0.30f, 0.0f, 0.022f, 0.85f, 0.70f, 0.00f, 0.60f },
    // RAINY: darker, wetter, a steady rain leaning with the wind.
    { 0.92f, 0.72f, 0.22f, 1.05f, 0.20f, 0.014f, 24.0f, 1.40f, 0.85f, 0.95f, 0.65f, 0.40f, 0.0f, 0.045f, 0.70f, 0.50f, 0.25f, 0.90f },
    // MISTY: little cloud, a great deal of haze - visibility drops to a few ship lengths.
    { 0.35f, 0.98f, 0.60f, 1.15f, 0.85f, 0.012f,  6.0f, 1.10f, 1.00f, 0.50f, 0.00f, 0.05f, 0.0f, 0.010f, 0.90f, 0.55f, 0.30f, 0.35f },
    // STORM: near-black sky, a downpour driven by a gale, lightning, and a heaving sea.
    { 1.00f, 0.40f, 0.10f, 1.15f, 0.15f, 0.018f, 24.0f, 1.70f, 0.55f, 1.00f, 1.00f, 1.00f, 1.0f, 0.110f, 0.50f, 0.40f, 0.70f, 2.00f },
};

// ---- the data: locations (their effect on the air; their scenery is in src/Scenery.h) ----------------------------------

struct LocationMod {
    float mist;           // fog this place has whatever the weather
    float hazeAdd;
    glm::vec3 waterKa, waterKd;    // the water's colour here (the open ocean's is Material.h's OCEAN, unchanged)
};

const LocationMod LOCATIONS[LOCATION_COUNT] = {
    // OPEN OCEAN: the previous build's sea.
    { 0.00f, 0.000f, glm::vec3(0.02f, 0.05f, 0.08f), glm::vec3(0.06f, 0.14f, 0.20f) },
    // JUNGLE ISLAND: warm, humid, turquoise shallows.
    { 0.00f, 0.004f, glm::vec3(0.02f, 0.09f, 0.10f), glm::vec3(0.05f, 0.28f, 0.30f) },
    // MOUNTAIN COAST: cold deep slate-blue.
    { 0.00f, 0.000f, glm::vec3(0.02f, 0.04f, 0.07f), glm::vec3(0.04f, 0.10f, 0.17f) },
    // ROCKY ISLANDS: green-grey, clear shallows over reefs.
    { 0.00f, 0.002f, glm::vec3(0.02f, 0.07f, 0.07f), glm::vec3(0.05f, 0.17f, 0.17f) },
    // FOGGY COAST: grey-green, always misty.
    { 0.42f, 0.006f, glm::vec3(0.03f, 0.05f, 0.05f), glm::vec3(0.07f, 0.12f, 0.12f) },
};

} // namespace EnvironmentData

// ---- the composition -------------------------------------------------------------------------------------------------

inline float environmentLuminance(const glm::vec3& c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

// The atmosphere for one choice. With the default choice (open ocean, sunny, sunset) every blend below is by exactly 0 and every scale
// exactly 1, so the result is the golden hour of Phase 43 to the last bit.
inline Atmosphere composeAtmosphereFrom(const TimeBase& t, int weather, int location)
{
    using namespace EnvironmentData;
    const WeatherMod& w = WEATHERS[std::clamp(weather, 0, WEATHER_COUNT - 1)];
    const LocationMod& l = LOCATIONS[std::clamp(location, 0, LOCATION_COUNT - 1)];

    const float over = w.overcast;
    const float mist = std::max(w.mist, l.mist);

    Atmosphere a;

    // Direct light: overcast scatters it, so it is desaturated (a sunset behind cloud is not orange) and weakened.
    a.sunDirection = t.sunDirection;
    a.sunColor = glm::mix(t.sunColor, glm::vec3(environmentLuminance(t.sunColor)), over * 0.6f) * w.sunScale;

    // The sky: clear colours blended towards overcast ones, darkened, then pulled towards the fog colour by mist.
    glm::vec3 horizon = glm::mix(t.horizon, t.overHorizon, over) * w.darken;
    glm::vec3 zenith = glm::mix(t.zenith, t.overZenith, over) * w.darken;
    horizon = glm::mix(horizon, t.fog, mist);
    zenith = glm::mix(zenith, t.fog * 0.9f, mist * 0.75f);
    a.horizon = horizon;
    a.zenith = zenith;

    // Ambient light is the sky's own light, so it follows the sky: overcast ambient, scaled, and toned towards the fog.
    a.ambient = glm::mix(glm::mix(t.ambient, t.overAmbient, over) * w.ambientScale, t.fog * 0.55f, mist * 0.5f);

    // Haze: the time's own, plus weather, plus the place, plus mist (the largest single term in a fog).
    a.hazeDensity = t.hazeDensity + w.hazeAdd + l.hazeAdd + mist * 0.014f;
    a.mist = mist;

    a.discColor = t.discColor;
    a.discVisible = std::clamp(1.0f - over * 1.1f, 0.0f, 1.0f) * (1.0f - mist * 0.7f);
    a.starAlpha = t.stars * (1.0f - over) * (1.0f - mist);

    a.cloudColor = glm::mix(t.cloudColor, t.overCloudColor, over) * w.cloudDark;
    a.cloudCount = w.cloudCount;
    a.cloudScale = w.cloudScale;
    a.cloudAlpha = w.cloudAlpha;

    a.rain = w.rain;
    a.wind = w.wind;
    a.lightning = w.lightning;

    a.seaRock = w.seaRock;
    a.waveAmp = w.wave;
    a.seaDark = w.seaDark * t.seaLift;
    a.seaShine = w.seaShine;
    a.seaSky = std::max(w.overcast, t.seaSky);

    a.lanterns = std::max(t.lanterns, w.lanternBoost);
    a.birds = t.birds * (1.0f - over) * (1.0f - mist) * (1.0f - w.rain);
    return a;
}

// The atmosphere for a choice of FIXED options (a dynamic option is read as the nearest fixed one; the live version is composeAtmosphereLive()
// below, which also needs the clock).
inline Atmosphere composeAtmosphere(const EnvironmentChoice& choice)
{
    return composeAtmosphereFrom(EnvironmentData::TIMES[std::clamp(choice.time, 0, TIME_COUNT - 1)], choice.weather, choice.location);
}

// A blend of two atmospheres. k = 0 is a, k = 1 is b. This is how a change of weather becomes a transition instead of a cut: the
// renderer holds the atmosphere it was showing, the one it is heading for and how far along it is.
inline Atmosphere mixAtmosphere(const Atmosphere& a, const Atmosphere& b, float k)
{
    Atmosphere r;
    const glm::vec3 d = glm::mix(a.sunDirection, b.sunDirection, k);
    r.sunDirection = (glm::dot(d, d) > 1e-6f) ? glm::normalize(d) : b.sunDirection;
    r.sunColor = glm::mix(a.sunColor, b.sunColor, k);
    r.ambient = glm::mix(a.ambient, b.ambient, k);
    r.horizon = glm::mix(a.horizon, b.horizon, k);
    r.zenith = glm::mix(a.zenith, b.zenith, k);
    r.mist = glm::mix(a.mist, b.mist, k);
    r.hazeDensity = glm::mix(a.hazeDensity, b.hazeDensity, k);
    r.discColor = glm::mix(a.discColor, b.discColor, k);
    r.discVisible = glm::mix(a.discVisible, b.discVisible, k);
    r.starAlpha = glm::mix(a.starAlpha, b.starAlpha, k);
    r.cloudColor = glm::mix(a.cloudColor, b.cloudColor, k);
    r.cloudCount = glm::mix(a.cloudCount, b.cloudCount, k);
    r.cloudScale = glm::mix(a.cloudScale, b.cloudScale, k);
    r.cloudAlpha = glm::mix(a.cloudAlpha, b.cloudAlpha, k);
    r.rain = glm::mix(a.rain, b.rain, k);
    r.wind = glm::mix(a.wind, b.wind, k);
    r.lightning = glm::mix(a.lightning, b.lightning, k);
    r.seaRock = glm::mix(a.seaRock, b.seaRock, k);
    r.waveAmp = glm::mix(a.waveAmp, b.waveAmp, k);
    r.seaDark = glm::mix(a.seaDark, b.seaDark, k);
    r.seaShine = glm::mix(a.seaShine, b.seaShine, k);
    r.seaSky = glm::mix(a.seaSky, b.seaSky, k);
    r.lanterns = glm::mix(a.lanterns, b.lanterns, k);
    r.birds = glm::mix(a.birds, b.birds, k);
    return r;
}

// Smoothstep: 0 to 1 with a zero slope at both ends, so a transition starts and stops gently (the camera's glide uses the same curve).
inline float environmentSmoothstep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ---- lightning --------------------------------------------------------------------------------------------------------

inline unsigned int environmentHash(unsigned int x)
{
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

struct LightningState {
    float flash = 0.0f;      // 0..1: how bright the sky is from the strike right now
    bool bolt = false;       // is the bolt itself visible
    float boltAge = 0.0f;
    unsigned int seed = 0;   // chooses where the bolt strikes and how it zigzags
};

// A function of the clock and nothing else: time is cut into 6 second periods, and in two of every three a strike happens at a time
// the period number chooses. A strike is a bright flash that dies away with a fainter second stroke 0.22 s later. Because it is a
// closed form of `now`, there is no stored schedule, and the same instant always gives the same sky.
inline LightningState lightningAt(float now)
{
    constexpr float PERIOD = 6.0f;
    LightningState s;
    const int block = static_cast<int>(std::floor(now / PERIOD));
    const unsigned int h = environmentHash(static_cast<unsigned int>(block) + 9137u);
    s.seed = h;
    if ((h >> 20) % 3u == 0u)
        return s;                                            // a quiet period
    const float start = static_cast<float>(block) * PERIOD + 0.6f + static_cast<float>(h % 1000u) / 1000.0f * 3.8f;
    const float t = now - start;
    if (t < 0.0f || t > 1.0f)
        return s;
    float f = std::exp(-t * 9.0f);
    if (t > 0.22f)
        f = std::max(f, 0.7f * std::exp(-(t - 0.22f) * 10.0f));
    s.flash = f;
    s.bolt = t < 0.30f;
    s.boltAge = t;
    return s;
}

// The atmosphere as the lightning lights it: the sky, the ambient and the "sun" slot all jump towards a cold white for the length of
// the flash. Applied AFTER the transition blend, so a flash during a change of weather is not carried into the next atmosphere.
inline Atmosphere withLightning(Atmosphere a, const LightningState& s)
{
    const float f = s.flash * a.lightning;
    if (f <= 0.0f)
        return a;
    a.horizon += glm::vec3(0.50f, 0.55f, 0.70f) * f * 0.8f;
    a.zenith += glm::vec3(0.45f, 0.50f, 0.65f) * f * 0.8f;
    a.ambient += glm::vec3(0.40f, 0.45f, 0.60f) * f * 0.45f;
    a.sunColor += glm::vec3(0.85f, 0.92f, 1.10f) * f * 0.9f;
    return a;
}

// ---- DYNAMIC: the sun that moves and the weather that changes by itself ---------------------------------------------------------------
//
// With TIME set to DYNAMIC a day passes in DynamicConfig::DAY_SECONDS of play. With WEATHER set to DYNAMIC a new weather arrives every
// DynamicConfig::WEATHER_SECONDS. Neither is a stored animation: the position of the sun is a cosine of the clock, and the weather in force
// is a hash of the number of the spell the clock is in, so the same instant always gives the same sky.
//
// THE DAY. A phase u from 0 to 1 (0 = noon). The sun's elevation is 50 degrees x cos(2 pi u), so it rises at u = 0.75, is highest at 0 and
// sets at 0.25; its bearing swings 50 degrees either way across the front of the ship (sin). The moon is the other half of the sky: it
// is up when the sun is down (elevation 30 x -cos) and crosses the same arc the other way. The light slot holds whichever is up; at the
// moment of the change, when neither is high, the light dips, which is dusk.
//
// WHAT THE SKY LOOKS LIKE is read off the sun's elevation, not off the clock: above 38 degrees it is the DAY row of the table, at 14 degrees it
// is the golden hour (the SUNSET row, the colours of Phase 43), below -8 degrees it is NIGHT, and in between the rows are blended. So a low sun is
// golden whether it is setting or rising, and a table of three looks becomes a continuous day.
namespace DynamicConfig {
constexpr float DAY_SECONDS = 240.0f;          // one whole day-night cycle (the viva value: make it 60 to watch a day in a minute)
constexpr float WEATHER_SECONDS = 40.0f;       // how long each spell of weather lasts
constexpr float WEATHER_BLEND_SECONDS = 10.0f; // the last part of a spell is spent changing into the next
constexpr float SUN_PEAK_DEGREES = 50.0f;
constexpr float MOON_PEAK_DEGREES = 30.0f;
constexpr float SUN_BEARING_DEGREES = 50.0f;
} // namespace DynamicConfig

// The origins of the two dynamic clocks, so that choosing DYNAMIC continues from where the scene is instead of jumping: the time of day starts
// at the phase of the time you were looking at, and the first spell of weather is the weather you were looking at.
struct DynamicClocks {
    float dayOrigin = 0.0f;           // the clock value at which the day's phase was 0 (noon)
    float weatherOrigin = 0.0f;       // the clock value at which the first spell began
    int startWeather = WeatherId::SUNNY;
    unsigned int seed = 1u;           // different runs, different weather
};

// The phase u (0..1, 0 = noon, 0.25 = sunset, 0.5 = midnight, 0.75 = dawn) of a FIXED time of day, used to start a dynamic day from it.
inline float dayPhaseOfFixedTime(int time)
{
    return time == TimeId::DAY ? 0.0f : (time == TimeId::SUNSET ? 0.22f : 0.5f);
}

inline TimeBase mixTimeBase(const TimeBase& a, const TimeBase& b, float k)
{
    TimeBase r;
    r.sunDirection = glm::mix(a.sunDirection, b.sunDirection, k);
    r.sunColor = glm::mix(a.sunColor, b.sunColor, k);
    r.ambient = glm::mix(a.ambient, b.ambient, k);
    r.horizon = glm::mix(a.horizon, b.horizon, k);
    r.zenith = glm::mix(a.zenith, b.zenith, k);
    r.hazeDensity = glm::mix(a.hazeDensity, b.hazeDensity, k);
    r.overHorizon = glm::mix(a.overHorizon, b.overHorizon, k);
    r.overZenith = glm::mix(a.overZenith, b.overZenith, k);
    r.overAmbient = glm::mix(a.overAmbient, b.overAmbient, k);
    r.fog = glm::mix(a.fog, b.fog, k);
    r.discColor = glm::mix(a.discColor, b.discColor, k);
    r.stars = glm::mix(a.stars, b.stars, k);
    r.cloudColor = glm::mix(a.cloudColor, b.cloudColor, k);
    r.overCloudColor = glm::mix(a.overCloudColor, b.overCloudColor, k);
    r.lanterns = glm::mix(a.lanterns, b.lanterns, k);
    r.birds = glm::mix(a.birds, b.birds, k);
    r.seaLift = glm::mix(a.seaLift, b.seaLift, k);
    r.seaSky = glm::mix(a.seaSky, b.seaSky, k);
    return r;
}

// The look of the sky for a sun at a given elevation (degrees): the table's three rows blended by how high the sun is.
inline TimeBase timeBaseForSunElevation(float elevationDegrees)
{
    using namespace EnvironmentData;
    const float towardSunset = environmentSmoothstep((38.0f - elevationDegrees) / 24.0f);     // 0 at 38 degrees and above, 1 at 14
    const float towardNight = environmentSmoothstep((14.0f - elevationDegrees) / 22.0f);      // 0 at 14 degrees and above, 1 at -8
    return mixTimeBase(mixTimeBase(TIMES[TimeId::DAY], TIMES[TimeId::SUNSET], towardSunset), TIMES[TimeId::NIGHT], towardNight);
}

// The weather of spell k of a dynamic run. Spell 0 is the weather you were looking at when you chose DYNAMIC; the rest are drawn from a hash of
// the spell's number and the run's seed, weighted so that fair weather is the most common and a storm the rarest.
inline int dynamicWeatherOfSpell(int spell, const DynamicClocks& dc)
{
    if (spell <= 0)
        return dc.startWeather;
    const unsigned int h = environmentHash(static_cast<unsigned int>(spell) * 2654435761u + dc.seed) % 100u;
    if (h < 30u) return WeatherId::SUNNY;
    if (h < 55u) return WeatherId::CLOUDY;
    if (h < 73u) return WeatherId::RAINY;
    if (h < 85u) return WeatherId::MISTY;
    return WeatherId::STORM;
}

// The phase of the day at clock value `now`: 0 at noon, wrapping every DAY_SECONDS.
inline float dynamicDayPhase(float now, const DynamicClocks& dc)
{
    const float u = (now - dc.dayOrigin) / DynamicConfig::DAY_SECONDS;
    return u - std::floor(u);
}

// Points the "sun slot" light, and the disc drawn in the sky, at whichever of the sun and the moon is up at phase u.
inline void placeSkyBody(Atmosphere& a, float u)
{
    constexpr float RAD = 3.14159265f / 180.0f;
    const float c = std::cos(6.2831853f * u), s = std::sin(6.2831853f * u);
    const float sunElevation = DynamicConfig::SUN_PEAK_DEGREES * c;
    const bool sunIsUp = sunElevation > -2.0f;
    const float elevation = sunIsUp ? sunElevation : -DynamicConfig::MOON_PEAK_DEGREES * c;
    const float bearing = (sunIsUp ? 1.0f : -1.0f) * DynamicConfig::SUN_BEARING_DEGREES * s;
    const glm::vec3 towardBody(std::cos(elevation * RAD) * std::sin(bearing * RAD), std::sin(elevation * RAD), std::cos(elevation * RAD) * std::cos(bearing * RAD));
    a.sunDirection = -towardBody;                      // the light TRAVELS away from the body

    // When the sun has just gone and the moon has not climbed yet, little light reaches the sea: dusk. The dip is deepest at the moment the
    // light changes hands and gone 10 degrees away from it.
    const float dip = std::clamp(std::fabs(sunElevation + 2.0f) / 10.0f, 0.10f, 1.0f);
    a.sunColor *= dip;
    a.discVisible *= environmentSmoothstep((elevation + 1.5f) / 4.5f);       // a disc below the horizon is not drawn
}

// The atmosphere NOW, for a choice that may contain DYNAMIC options. For a choice of fixed options it is composeAtmosphere(choice).
inline Atmosphere composeAtmosphereLive(const EnvironmentChoice& choice, float now, const DynamicClocks& dc)
{
    const bool dynamicTime = choice.time >= TIME_DYNAMIC;
    const bool dynamicWeather = choice.weather >= WEATHER_DYNAMIC;

    TimeBase t = EnvironmentData::TIMES[std::clamp(choice.time, 0, TIME_COUNT - 1)];
    float phase = 0.0f;
    if (dynamicTime) {
        phase = dynamicDayPhase(now, dc);
        t = timeBaseForSunElevation(DynamicConfig::SUN_PEAK_DEGREES * std::cos(6.2831853f * phase));
    }
    const auto atmosphereFor = [&](int weather) {
        Atmosphere a = composeAtmosphereFrom(t, weather, choice.location);
        if (dynamicTime)
            placeSkyBody(a, phase);
        return a;
    };

    if (!dynamicWeather)
        return atmosphereFor(choice.weather);

    const float spells = (now - dc.weatherOrigin) / DynamicConfig::WEATHER_SECONDS;
    const int spell = static_cast<int>(std::floor(spells));
    const float seconds = (spells - static_cast<float>(spell)) * DynamicConfig::WEATHER_SECONDS;
    const float blend = environmentSmoothstep((seconds - (DynamicConfig::WEATHER_SECONDS - DynamicConfig::WEATHER_BLEND_SECONDS)) / DynamicConfig::WEATHER_BLEND_SECONDS);
    const Atmosphere current = atmosphereFor(dynamicWeatherOfSpell(spell, dc));
    if (blend <= 0.0f)
        return current;
    return mixAtmosphere(current, atmosphereFor(dynamicWeatherOfSpell(spell + 1, dc)), blend);
}

// ---- the sea, as a material -----------------------------------------------------------------------------------------

// The ocean's material here and now: the location's water colours, darkened and roughened by the weather. With the defaults it IS
// the OCEAN material of Phase 29 (same four numbers), so the open ocean at sunset looks as it always did.
inline Material seaMaterialFor(int location, const Atmosphere& a)
{
    const EnvironmentData::LocationMod& l = EnvironmentData::LOCATIONS[std::clamp(location, 0, LOCATION_COUNT - 1)];
    Material m = OCEAN;
    m.ka = l.waterKa * a.seaDark * (1.0f + 5.0f * a.seaSky);       // under a grey sky or a night sky the water gives back the sky's light
    m.kd = l.waterKd * a.seaDark;
    m.ks = OCEAN.ks * glm::mix(0.35f, 1.0f, a.seaShine);     // a rough sea scatters the glitter into a dull sheen
    m.ns = glm::mix(40.0f, OCEAN.ns, a.seaShine);
    return m;
}
