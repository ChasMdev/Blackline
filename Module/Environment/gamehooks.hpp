#pragma once
#include "Http/Http.h"

namespace GameHooks {

    static lua_CFunction OriginalNamecall;
    static lua_CFunction OriginalIndex;

    inline std::vector<const char*> dangerousFunctions =
    {
        ("OpenVideosFolder"), ("OpenScreenshotsFolder"),
        ("GetRobuxBalance"), ("PerformPurchase"), ("PromptBundlePurchase"), ("PromptNativePurchase"),
        ("PromptProductPurchase"), ("PromptPurchase"), ("PromptThirdPartyPurchase"), ("Publish"),
        ("GetMessageId"), ("OpenBrowserWindow"), ("RequestInternal"), ("ExecuteJavaScript"),
        ("openvideosfolder"), ("openscreenshotsfolder"), ("getrobuxbalance"), ("performpurchase"),
        ("promptbundlepurchase"), ("promptnativepurchase"), ("promptproductpurchase"),
        ("promptpurchase"), ("promptthirdpartypurchase"), ("publish"), ("getmessageid"),
        ("openbrowserwindow"), ("requestinternal"), ("executejavascript"), ("openVideosFolder"),
        ("openScreenshotsFolder"), ("getRobuxBalance"), ("performPurchase"), ("promptBundlePurchase"),
        ("promptNativePurchase"), ("promptProductPurchase"), ("promptPurchase"),
        ("promptThirdPartyPurchase"), ("publish"), ("getMessageId"), ("openBrowserWindow"),
        ("requestInternal"), ("executeJavaScript"),
        ("ToggleRecording"), ("TakeScreenshot"), ("HttpRequestAsync"), ("GetLast"),
        ("SendCommand"), ("GetAsync"), ("GetAsyncFullUrl"), ("RequestAsync"), ("MakeRequest"),
        ("togglerecording"), ("takescreenshot"), ("httprequestasync"), ("getlast"),
        ("sendcommand"), ("getasync"), ("getasyncfullurl"), ("requestasync"), ("makerequest"),
        ("toggleRecording"), ("takeScreenshot"), ("httpRequestAsync"), ("getLast"),
        ("sendCommand"), ("getAsync"), ("getAsyncFullUrl"), ("requestAsync"), ("makeRequest"),
        ("AddCoreScriptLocal"),
        ("SaveScriptProfilingData"), ("GetUserSubscriptionDetailsInternalAsync"),
        ("GetUserSubscriptionStatusAsync"), ("PerformBulkPurchase"), ("PerformCancelSubscription"),
        ("PerformPurchaseV2"), ("PerformSubscriptionPurchase"), ("PerformSubscriptionPurchaseV2"),
        ("PrepareCollectiblesPurchase"), ("PromptBulkPurchase"), ("PromptCancelSubscription"),
        ("PromptCollectiblesPurchase"), ("PromptGamePassPurchase"), ("PromptNativePurchaseWithLocalPlayer"),
        ("PromptPremiumPurchase"), ("PromptRobloxPurchase"), ("PromptSubscriptionPurchase"),
        ("ReportAbuse"), ("ReportAbuseV3"), ("ReturnToJavaScript"), ("OpenNativeOverlay"),
        ("OpenWeChatAuthWindow"), ("EmitHybridEvent"), ("OpenUrl"), ("PostAsync"), ("PostAsyncFullUrl"),
        ("RequestLimitedAsync"), ("Run"), ("Load"), ("CaptureScreenshot"), ("CreatePostAsync"),
        ("DeleteCapture"), ("DeleteCapturesAsync"), ("GetCaptureFilePathAsync"), ("SaveCaptureToExternalStorage"),
        ("SaveCapturesToExternalStorageAsync"), ("GetCaptureUploadDataAsync"), ("RetrieveCaptures"),
        ("SaveScreenshotCapture"), ("Call"), ("GetProtocolMethodRequestMessageId"),
        ("GetProtocolMethodResponseMessageId"), ("PublishProtocolMethodRequest"),
        ("PublishProtocolMethodResponse"), ("Subscribe"), ("SubscribeToProtocolMethodRequest"),
        ("SubscribeToProtocolMethodResponse"), ("GetDeviceIntegrityToken"), ("GetDeviceIntegrityTokenYield"),
        ("NoPromptCreateOutfit"), ("NoPromptDeleteOutfit"), ("NoPromptRenameOutfit"), ("NoPromptSaveAvatar"),
        ("NoPromptSaveAvatarThumbnailCustomization"), ("NoPromptSetFavorite"), ("NoPromptUpdateOutfit"),
        ("PerformCreateOutfitWithDescription"), ("PerformRenameOutfit"), ("PerformSaveAvatarWithDescription"),
        ("PerformSetFavorite"), ("PerformUpdateOutfit"), ("PromptCreateOutfit"), ("PromptDeleteOutfit"),
        ("PromptRenameOutfit"), ("PromptSaveAvatar"), ("PromptSetFavorite"), ("PromptUpdateOutfit")
    };


    inline int NewNamecall(lua_State* L) {
        if (!L)
            return 0;

        if (Taskscheduler->CheckThread(L) && L->namecall->data != nullptr) {
            const char* data = L->namecall->data;

            if (!data) return 0;

            if (strcmp(data, "HttpGet") == 0 || strcmp(data, "HttpGetAsync") == 0) {
                return Http::httpget(L);
            }

            for (const std::string& func : dangerousFunctions) {
                if (std::string(data) == func) {
                    luaL_error(L, "Function has been disabled for security reasons.");
                    return 0;
                }
            }
        }

        if (!OriginalNamecall)
            return 0;

        return OriginalNamecall(L);
    }

    inline int NewIndex(lua_State* L) {
        if (!L)
            return 0;

        if (Taskscheduler->CheckThread(L) && !std::string(lua_tostring(L, 2)).empty()) {
            const char* data = lua_tostring(L, 2);

            if (!data) return 0;

            if (strcmp(data, ("HttpGet")) == 0 || strcmp(data, ("HttpGetAsync")) == 0) {
                lua_getglobal(L, ("HttpGet"));
                return 1;
            }

            for (const std::string& func : dangerousFunctions) {
                if (std::string(data) == func) {
                    luaL_error(L, ("Function has been disabled for security reasons."));
                    return 0;
                }
            }
        }

        if (!OriginalIndex)
            return 0;

        return OriginalIndex(L);
    }

    inline void InitializeHooks(lua_State* L) {
        int originalCount = lua_gettop(L);

        lua_getglobal(L, "game");
        luaL_getmetafield(L, -1, "__index");

        Closure* Index = clvalue(luaA_toobject(L, -1));
        OriginalIndex = Index->c.f;
        Index->c.f = NewIndex;

        lua_pop(L, 1);

        luaL_getmetafield(L, -1, "__namecall");

        Closure* Namecall = clvalue(luaA_toobject(L, -1));
        OriginalNamecall = Namecall->c.f;
        Namecall->c.f = NewNamecall;

        lua_settop(L, originalCount);
    }

};