#include "../include/config.hpp"
#include "../../beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "../../beatsaber-hook/rapidjson/include/rapidjson/allocators.h"
#include "../../beatsaber-hook/rapidjson/include/rapidjson/document.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#define COLOR_ARRAY_LENGTH 4


std::vector<segment> getSegments(rapidjson::Value& arr) {
    std::vector<segment> out;
    auto size = arr.Size();
    for (int i = 0; i < size; i++) {
        auto& currentValue = arr[i];
        segment toAdd;
        toAdd.threshold = currentValue["threshold"].GetInt();
        toAdd.text = currentValue["text"].GetString();
        out.push_back(toAdd);
    }
    return out;
}

std::vector<judgement> getJudgements(rapidjson::Value& arr) {
    std::vector<judgement> out;
    auto size = arr.Size();
    for (int i = 0; i < size; i++) {
        auto& currentValue = arr[i];
        judgement toAdd;
        toAdd.threshold = currentValue["threshold"].GetInt();
        toAdd.text = currentValue["text"].GetString();
        toAdd.color = std::vector<float>();
        for (int j = 0; j < COLOR_ARRAY_LENGTH; j++) {
            toAdd.color.push_back(currentValue["color"][j].GetFloat());
        }
        auto itr = currentValue.FindMember("fade");
        if (itr != currentValue.MemberEnd()) {
            toAdd.fade = itr->value.GetBool();
        }
        out.push_back(toAdd);
    }
    return out;
}

const char* convertDisplayMode(DisplayMode displayMode) {
    switch (displayMode) {
        case DISPLAY_MODE_FORMAT:
            return "format";
        case DISPLAY_MODE_NUMERIC:
            return "numeric";
        case DISPLAY_MODE_SCOREONTOP:
            return "scoreOnTop";
        case DISPLAY_MODE_TEXTONLY:
            return "textOnly";
        case DISPLAY_MODE_TEXTONTOP:
            return "textOnTop";
    }
    return "UNKNOWN";
}

DisplayMode convertDisplayMode(std::string_view displayMode) {
    if (displayMode == "format") {
        return DISPLAY_MODE_FORMAT;
    } else if (displayMode == "numeric") {
        return DISPLAY_MODE_NUMERIC;
    } else if (displayMode == "scoreOnTop") {
        return DISPLAY_MODE_SCOREONTOP;
    } else if (displayMode == "textOnly") {
        return DISPLAY_MODE_TEXTONLY;
    } else if (displayMode == "textOnTop") {
        return DISPLAY_MODE_TEXTONTOP;
    }
    // Otherwise, display in default: format
    return DISPLAY_MODE_FORMAT;
}

void ConfigHelper::AddJSONJudgement(rapidjson::MemoryPoolAllocator<>& allocator, rapidjson::Document::ValueType& arr, judgement& j) {
    rapidjson::Value v(rapidjson::kObjectType);
    v.AddMember("threshold", j.threshold, allocator);
    v.AddMember("text", rapidjson::GenericStringRef<char>(j.text), allocator);
    auto color = rapidjson::Value(rapidjson::kArrayType).GetArray();
    for (int i = 0; i < COLOR_ARRAY_LENGTH; i++) {
        color.PushBack(j.color[i], allocator);
    }
    v.AddMember("color", color, allocator);
    v.AddMember("fade", j.fade, allocator);
    arr.PushBack(v, allocator);
}

void ConfigHelper::AddJSONSegment(rapidjson::Document::AllocatorType& allocator, rapidjson::Document::ValueType& arr, segment& s) {
    rapidjson::Value v(rapidjson::kObjectType);
    v.AddMember("threshold", s.threshold, allocator);
    v.AddMember("text", rapidjson::GenericStringRef<char>(s.text), allocator);
    arr.PushBack(v, allocator);
}

void ConfigHelper::BackupConfig(ConfigDocument& config, std::string_view newPath) {
    if (!direxists(CONFIG_PATH)) {
        mkdir(CONFIG_PATH, 0700);
    }
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    config.Accept(writer);
    writefile(newPath.data(), buf.GetString());
}

void ConfigHelper::RestoreConfig(std::string_view newPath) {
    if (fileexists(newPath.data())) {
        writefile(getconfigpath().data(), readfile(newPath.data()));
    }
    Configuration::Reload();
}

void ConfigHelper::CreateJSONSegments(rapidjson::MemoryPoolAllocator<>& allocator, ConfigDocument& config, std::vector<segment>& vector, std::string_view name) {
    auto arr = rapidjson::Value(rapidjson::kArrayType);
    for (auto itr = vector.begin(); itr != vector.end(); ++itr) {
        ConfigHelper::AddJSONSegment(allocator, arr, *itr);
    }
    config.AddMember(rapidjson::GenericStringRef<char>(name.data()), arr, allocator);
}

void ConfigHelper::CreateJudgement(std::vector<judgement>& judgements, int index, int threshold, std::string_view text, std::vector<float> color, bool fade) {
    judgements[index].threshold = threshold;
    judgements[index].text = text.data();
    judgements[index].color = color;
    judgements[index].fade = fade;
}

void ConfigHelper::CreateSegment(std::vector<segment>& segments, int index, int threshold, std::string_view text) {
    segments[index].threshold = threshold;
    segments[index].text = text.data();
}

HSVConfig ConfigHelper::LoadConfig(ConfigDocument& config) {
    HSVConfig con;
    if (!config.IsObject() || config.MemberCount() < 11) {
        con.SetToDefault();
        return con;
    }
    con.majorVersion = config["majorVersion"].GetInt();
    con.minorVersion = config["minorVersion"].GetInt();
    con.patchVersion = config["patchVersion"].GetInt();
    con.judgements = getJudgements(config["judgements"]);
    con.beforeCutAngleJudgements = getSegments(config["beforeCutAngleJudgements"]);
    con.accuracyJudgements = getSegments(config["accuracyJudgements"]);
    con.afterCutAngleJudgements = getSegments(config["afterCutAngleJudgements"]);
    con.type = (ConfigType)config["type"].GetInt();
    con.useSeasonalThemes = config["useSeasonalThemes"].GetBool();
    con.backupBeforeSeason = config["backupBeforeSeason"].GetBool();
    con.restoreAfterSeason = config["restoreAfterSeason"].GetBool();
    con.displayMode = convertDisplayMode(config["displayMode"].GetString());
    return con;
}

void HSVConfig::WriteToConfig(ConfigDocument& config) {
    config.SetObject();
    config.RemoveAllMembers();
    rapidjson::MemoryPoolAllocator<>& allocator = config.GetAllocator();
    // Add versions
    config.AddMember("majorVersion", majorVersion, allocator);
    config.AddMember("minorVersion", minorVersion, allocator);
    config.AddMember("patchVersion", patchVersion, allocator);
    auto arr = rapidjson::Value(rapidjson::kArrayType);
    // Add judgements
    for (auto itr = judgements.begin(); itr != judgements.end(); ++itr) {
        ConfigHelper::AddJSONJudgement(allocator, arr, *itr);
    }
    config.AddMember("judgements", arr, allocator);
    // Add segments
    ConfigHelper::CreateJSONSegments(allocator, config, beforeCutAngleJudgements, "beforeCutAngleJudgements");
    ConfigHelper::CreateJSONSegments(allocator, config, accuracyJudgements, "accuracyJudgements");
    ConfigHelper::CreateJSONSegments(allocator, config, afterCutAngleJudgements, "afterCutAngleJudgements");
    // Set metadata
    config.AddMember("type", type, allocator); // Type can be: 0: Standard, 1: Christmas, 2: Easter, etc.
    config.AddMember("useSeasonalThemes", useSeasonalThemes, allocator);
    config.AddMember("backupBeforeSeason", backupBeforeSeason, allocator);
    config.AddMember("restoreAfterSeason", restoreAfterSeason, allocator);
    config.AddMember("displayMode", rapidjson::GenericStringRef<char>(convertDisplayMode(displayMode)), allocator);
}

void HSVConfig::SetToDefault() {
    majorVersion = 2;
    minorVersion = 2;
    patchVersion = 0;
    judgements = std::vector<judgement>(6);
    ConfigHelper::CreateJudgement(judgements, 0, 115, "%BFantastic%A%n%s", {1.0, 1.0, 1.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 1, 101, "<size=80%>%BExcellent%A</size>%n%s", {0.0, 1.0, 0.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 2, 90, "<size=80%>%BGreat%A</size>%n%s", {1.0, 0.980392158, 0.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 3, 80, "<size=80%>%BGood%A</size>%n%s", {1.0, 0.6, 0.0, 1.0}, true);
    ConfigHelper::CreateJudgement(judgements, 4, 60, "<size=80%>%BDecent%A</size>%n%s", {1.0, 0.0, 0.0, 1.0}, true);
    ConfigHelper::CreateJudgement(judgements, 5, 0, "<size=80%>%BWay Off%A</size>%n%s", {0.5, 0.0, 0.0, 1.0}, true);
    beforeCutAngleJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(beforeCutAngleJudgements, 0, 70, "+");
    ConfigHelper::CreateSegment(beforeCutAngleJudgements, 1, 0, " ");
    accuracyJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(accuracyJudgements, 0, 15, "+");
    ConfigHelper::CreateSegment(accuracyJudgements, 1, 0, " ");
    afterCutAngleJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(afterCutAngleJudgements, 0, 30, "+");
    ConfigHelper::CreateSegment(afterCutAngleJudgements, 1, 0, " ");
    type = CONFIG_TYPE_STANDARD;
    displayMode = DISPLAY_MODE_FORMAT;
    useSeasonalThemes = true;
    backupBeforeSeason = true;
    restoreAfterSeason = true;
}

void HSVConfig::SetToChristmas() {
    // Either download the config from somewhere (could be neat)
    // Or, Set it directly
    // Save these parameters to restore
    majorVersion = 2;
    minorVersion = 3;
    patchVersion = 0;
    judgements = std::vector<judgement>(6);
    ConfigHelper::CreateJudgement(judgements, 0, 115, "%BFantastic%A%n%s", {1.0, 1.0, 1.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 1, 101, "<size=80%>%BExcellent%A</size>%n%s", {0.0, 1.0, 0.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 2, 90, "<size=80%>%BGreat%A</size>%n%s", {1.0, 0.980392158, 0.0, 1.0});
    ConfigHelper::CreateJudgement(judgements, 3, 80, "<size=80%>%BGood%A</size>%n%s", {1.0, 0.6, 0.0, 1.0}, true);
    ConfigHelper::CreateJudgement(judgements, 4, 60, "<size=80%>%BDecent%A</size>%n%s", {1.0, 0.0, 0.0, 1.0}, true);
    ConfigHelper::CreateJudgement(judgements, 5, 0, "<size=80%>%BWay Off%A</size>%n%s", {0.5, 0.0, 0.0, 1.0}, true);
    beforeCutAngleJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(beforeCutAngleJudgements, 0, 70, "+");
    ConfigHelper::CreateSegment(beforeCutAngleJudgements, 1, 0, " ");
    accuracyJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(accuracyJudgements, 0, 15, "+");
    ConfigHelper::CreateSegment(accuracyJudgements, 1, 0, " ");
    afterCutAngleJudgements = std::vector<segment>(2);
    ConfigHelper::CreateSegment(afterCutAngleJudgements, 0, 30, "+");
    ConfigHelper::CreateSegment(afterCutAngleJudgements, 1, 0, " ");
    // Add particle effects
    // COMING SOON
    // Add UI effects
    // COMING SOON
    type = CONFIG_TYPE_CHRISTMAS;
    backupBeforeSeason = true;
    displayMode = DISPLAY_MODE_FORMAT;
}