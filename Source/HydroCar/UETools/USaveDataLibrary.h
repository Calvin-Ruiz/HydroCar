#pragma once

#include "CoreMinimal.h"
#include "FSaveData.h"
#include "../Tools/SaveData.hpp"
#include <chrono>
#include "USaveDataLibrary.generated.h"

USTRUCT(BlueprintType)
struct FSaveDataRoot {
    GENERATED_BODY()
private:
    SaveData base;
public:
    UPROPERTY(BlueprintReadWrite)
    FSaveData root = FSaveData{&base};
};

enum ManipTypeFlagBits {
    // Sign
    MT_SIGNED = 0x00,
    MT_UNSIGNED = 0x01,
    // Type
    MT_CHAR = 0x02,
    MT_SHORT = 0x04,
    MT_INT = 0x06,
    MT_LONG = 0x08,
    MT_FLOAT = 0x0a,
    MT_DOUBLE = 0x0c,
    MT_LONG_DOUBLE = 0x0e,
    MT_TYPE = 0x0f,
    // Layout
    MT_1_COMPONENTS = 0x00, // This is the unique layout which support operations after OP_ASSIGN
    MT_2_COMPONENTS = 0x10,
    MT_3_COMPONENTS = 0x20,
    MT_4_COMPONENTS = 0x30,
    MT_3_3_MATRIX = 0x40,
    MT_4_4_MATRIX = 0x50,
    MT_ARRAY = 0x60,
    MT_LAYOUT = 0x70,
    // Set if this is direct reference to the section targeted by the last OP_LOAD (not implemented yet)
    MT_REF = 0x80,
};

UENUM()
enum class MT : uint8 {
    CHAR = MT_CHAR,
    UCHAR = MT_UNSIGNED | MT_CHAR,
    SHORT = MT_SHORT,
    USHORT = MT_UNSIGNED | MT_SHORT,
    INT = MT_INT,
    UINT = MT_UNSIGNED | MT_INT,
    LONG = MT_LONG,
    ULONG = MT_UNSIGNED | MT_LONG,
    FLOAT = MT_FLOAT,
    DOUBLE = MT_DOUBLE,
    LONG_DOUBLE = MT_LONG_DOUBLE,
    VEC3 = MT_DOUBLE | MT_3_COMPONENTS,
    VEC4 = MT_DOUBLE | MT_4_COMPONENTS,
    MAT3 = MT_DOUBLE | MT_3_3_MATRIX,
    MAT4 = MT_DOUBLE | MT_4_4_MATRIX,
    STRING = MT_CHAR | MT_ARRAY,
};

#define self (*wrapper.handle)

UCLASS()
class HYDROCAR_API USaveDataLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
    UFUNCTION(Category = SaveData, BlueprintPure)
    static FSaveData AtIndex(FSaveData wrapper, int idx) {
        return {&self[idx]};
    }
    UFUNCTION(Category = SaveData, BlueprintPure)
    static FSaveData AtEntry(FSaveData wrapper, const FString &entry) {
        return {&self[TCHAR_TO_UTF8(*entry)]};
    }
    UFUNCTION(Category = SaveData, BlueprintPure)
    static int Size(FSaveData wrapper) {
        return self.size();
    }
    // Get value as MT::STRING type
    UFUNCTION(Category = SaveData, BlueprintPure)
    static FString AsString(FSaveData wrapper) {
        return FString(self.get().size(), UTF8_TO_TCHAR(self.get().data()));
    }
    // Get value as MT::VEC3 type
    UFUNCTION(Category = SaveData, BlueprintPure)
    static FVector AsVector(FSaveData wrapper) {
        return self.get<FVector>();
    }
    // Get value as MT::VEC3 type
    UFUNCTION(Category = SaveData, BlueprintPure)
    static FRotator AsRotator(FSaveData wrapper) {
        return self.get<FRotator>();
    }
    // Get value as MT::FLOAT type
    UFUNCTION(Category = SaveData, BlueprintPure)
    static float AsFloat(FSaveData wrapper) {
        return self.get<float>();
    }
    // Get value as any type from MT::CHAR to MT::INT
    UFUNCTION(Category = SaveData, BlueprintPure)
    static int AsInt(FSaveData wrapper, MT type) {
        switch (type) {
            case MT::CHAR:
                return self.get<int8_t>();
            case MT::UCHAR:
                return self.get<uint8_t>();
            case MT::SHORT:
                return self.get<int16_t>();
            case MT::USHORT:
                return self.get<uint16_t>();
            case MT::INT:
                return self.get<int32_t>();
        }
        return 0;
    }
    // Get value as MT::LONG or MT::ULONG
    UFUNCTION(Category = SaveData, BlueprintPure)
    static int64 AsLong(FSaveData wrapper, MT type) {
        return (type == MT::UINT) ? self.get<uint32_t>() : self.get<int64>();
    }
    // Get boolean as MT::CHAR type
    UFUNCTION(Category = SaveData, BlueprintPure)
    static bool AsBool(FSaveData wrapper) {
        return self.get<bool>();
    }
    // Get value as ellapsed time in milliseconds
    UFUNCTION(Category = SaveData, BlueprintPure)
    static int64 AsDuration(FSaveData wrapper) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(self.get<std::chrono::steady_clock::duration>()).count();
    }
    // Add an item to the list and return his index
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static int Add(FSaveData wrapper, FSaveData element) {
        if (element.handle)
            return self.push(*(element.handle));
        return self.push();
    }
    // Store as type MT::STRING
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetString(FSaveData wrapper, const FString &value) {
        const char *tmp = TCHAR_TO_UTF8(*value);
        std::vector<char> &vec = self.get();
        vec.clear();
        while (*tmp)
            vec.push_back(*(tmp++));
    }
    // Store as type MT::VEC3
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetVector(FSaveData wrapper, const FVector &value) {
        self.get<FVector>() = value;
    }
    // Store as type MT::VEC3
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetRotator(FSaveData wrapper, const FRotator &value) {
        self.get<FRotator>() = value;
    }
    // Store as type MT::FLOAT
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetFloat(FSaveData wrapper, float value) {
        self.get<float>() = value;
    }
    // Store as type from MT::CHAR to MT::INT
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetInt(FSaveData wrapper, int value, MT type) {
        switch (type) {
            case MT::CHAR:
                self.get<int8_t>() = value;
                break;
            case MT::UCHAR:
                self.get<uint8_t>() = value;
                break;
            case MT::SHORT:
                self.get<int16_t>() = value;
                break;
            case MT::USHORT:
                self.get<uint16_t>() = value;
                break;
            case MT::INT:
                self.get<int32_t>() = value;
                break;
        }
    }
    // Store as type from MT::UINT to MT::ULONG
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetLong(FSaveData wrapper, int64 value, MT type) {
        if (type == MT::UINT)
            self.get<uint32_t>() = value;
        else
            self.get<int64_t>() = value;
    }
    // Store boolean as type MT::CHAR
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void SetBool(FSaveData wrapper, bool value) {
        self.get<bool>() = value;
    }
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static FSaveDataRoot Create() {
        return {};
    }
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static FSaveDataRoot Load(TArray<uint8> datas) {
        FSaveDataRoot ret;
        char *ptr = (char *) &datas[0];
        ret.root.handle->load(ptr);
        return ret;
    }
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static TArray<uint8> Save(FSaveData wrapper) {
        TArray<uint8> ret;
        ret.SetNumUninitialized(self.computeSize());
        self.save((char *) &ret[0]);
        return ret;
    }
    UFUNCTION(Category = SaveData, BlueprintCallable)
    static void Assign(FSaveData wrapper, FSaveData value) {
        self = *value.handle;
    }
};

#undef self
