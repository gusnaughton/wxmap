#include <NeoPixelBus.h>
#include "animationController.h"
#include "animations/blink.h"
#include "animations/ceiling.h"
#include "animations/temperature.h"
#include "animations/visibility.h"
#include "animations/wind.h"
#include "ArduinoJson.h"
#include "poller.h"

JsonVariant clone(JsonBuffer &jb, JsonVariant prototype)
{
    if (prototype.is<JsonObject>())
    {
        const JsonObject &protoObj = prototype;
        JsonObject &newObj = jb.createObject();
        for (const auto &kvp : protoObj)
        {
            newObj[jb.strdup(kvp.key)] = clone(jb, kvp.value);
        }
        return newObj;
    }

    if (prototype.is<JsonArray>())
    {
        const JsonArray &protoArr = prototype;
        JsonArray &newArr = jb.createArray();
        for (const auto &elem : protoArr)
        {
            newArr.add(clone(jb, elem));
        }
        return newArr;
    }

    if (prototype.is<char *>())
    {
        return jb.strdup(prototype.as<const char *>());
    }

    return prototype;
}

AnimationController::AnimationController(uint16_t pixelCountIn, bool gammaSetting)
{
    wxData = new wxData_t{};
    shouldFetch = false;
    Serial.println(F("DEBUG init poller"));
    poller = new Poller(wxData);
    Serial.println(F("AnimationController::AnimationController(...): Call"));
    gamma = gammaSetting;
    Serial.println(F("AnimationController::AnimationController(...): init strip"));
    PixelCountChanged(pixelCountIn);

    Serial.println(F("AnimationController::AnimationController(...): gen test config"));
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["duration"] = 1000;

    Serial.println(F("AnimationController::AnimationController(...): init blink"));
    queue(0, root);
    cut();
    //currentAnimation = new Blink(root, strip);
}

void AnimationController::PixelCountChanged(uint16_t pixelCount)
{
    Serial.print(F("AnimationController::PixelCountCh4anged(...): pixelcount is"));
    Serial.println(pixelCount);
    if (strip != NULL)
    {
        Serial.println(F("AnimationController::PixelCountChanged(...): strip is not NULL"));
        delete strip;
    }
    Serial.println(F("AnimationController::PixelCountChanged(...): init strip"));
    strip = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>(pixelCount);
    Serial.println(F("AnimationController::PixelCountChanged(...): begin strip"));
    //TODO: debug
    strip->Begin();
    strip->ClearTo(1);
    strip->Show();
    delay(1000);
}

void AnimationController::update()
{
    float_t animProg = (millis() % currentAnimation->getDuration()) / (float_t)currentAnimation->getDuration();
    currentAnimation->render(wxData, animProg);
}

void AnimationController::reloadData()
{
    wxData = new wxData_t{};
    poller->sendRequest();
}

void AnimationController::setShouldFetch(bool value)
{
    Serial.println(F("AnimationController::setShouldFetch(...)"));
    shouldFetch = value;
    if (shouldFetch)
        poller->start();
}

void AnimationController::queue(int animationIndex, JsonObject &cfg)
{
    Serial.println(F("AnimationController::queue(...)"));
    nextAnimation = animationFactory(animationIndex, cfg);
}

void AnimationController::cut()
{
    Serial.println(F("AnimationController::cut(...)"));
    auto oldAnimation = currentAnimation;
    currentAnimation = nextAnimation;
}
// later: crossfade? new Crossfade(cfg, strip, animA, animB)

Animation *AnimationController::animationFactory(int animationIndex, JsonObject &cfg)
{
    auto safeCfg = clone(cfgBuf, cfg);
    switch (animationIndex)
    {
    //ceil
    case 0:
    {
        return new Ceiling(safeCfg, strip);
        break;
    }
    //temperature
    case 1:
    {
        return new Temperature(safeCfg, strip);
        break;
    }
    //visibility
    case 2:
    {
        return new Visibility(safeCfg, strip);
        break;
    }
    //wind
    case 3:
    {
        return new Wind(safeCfg, strip);
        break;
    }
    //blink
    case 255:
    {
        return new Blink(safeCfg, strip);
        break;
    }
    }
    return NULL;
}