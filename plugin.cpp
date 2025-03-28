#include <stdafx.h>
#include <detours.h>
#include <cJSON.h>
#include <clocale>
#include <fstream>
#include "plugin.h"
#include "rugp.h"
#include "hook.h"

static BOOL CreateMergeDirectory();

static CVmCommand* __fastcall Merge(const CVmCommand* ecx, cJSON* edx);

static AFX_EXTENSION_MODULE R514783_PLUGIN = {FALSE, nullptr};

// ReSharper disable once CppParameterMayBeConst
BOOL WINAPI DllMain(HINSTANCE hInstance, const DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        AllocConsole();
        SetConsoleTitleA("r514783 Plugin Debug Console");
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        setlocale(LC_ALL, ".UTF8");
        freopen("CON", "w", stdout);

        wprintf(L"MFC Version %hs\n", GetMfcVersion());
        wprintf(L"rUGP System Version %hs\n", GetRugpVersion());
        wprintf(L"cJSON Version %hs\n", cJSON_Version());
        wprintf(L"Detours Version %x\n", DETOURS_VERSION);
        wprintf(L"\n");
        wprintf(L"CommandLine %s\n", GetCommandLineW());
        wprintf(L"\n");

        AfxInitExtensionModule(R514783_PLUGIN, hInstance);

        CObjectProxy::LoadFromModule("UnivUI");
        CObjectProxy::LoadFromModule("rvmm");
        CObjectProxy::LoadFromModule("Vm60");

        __try
        {
            CreateMergeDirectory();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"CreateMergeDirectory Fail: 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            Win32Hook::AttachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"Win32Hook::AttachHook Fail: 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            CObjectProxy::AttachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"CObjectProxy::AttachHook Fail: 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            COceanTree::AttachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"COceanTree::AttachHook Fail: 0x%08X\n\n", GetExceptionCode());
        }

        break;
    case DLL_PROCESS_DETACH:
        __try
        {
            COceanTree::DetachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"COceanTree::DetachHook Fail: 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            CObjectProxy::DetachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"CObjectProxy::DetachHook 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            CObjectProxy::Clear();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"CObjectProxy::Clear 0x%08X\n\n", GetExceptionCode());
        }

        __try
        {
            Win32Hook::DetachHook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wprintf(L"Win32Hook::DetachHook Fail: 0x%08X\n\n", GetExceptionCode());
        }

        FreeConsole();

        AfxTermExtensionModule(R514783_PLUGIN, FALSE);

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}

const AFX_EXTENSION_MODULE* PluginThisLibrary()
{
    // CObjectProxy::LoadFromModule("riooor.rpo");
    // CObjectProxy::LoadFromModule("age_classic.rpo");

    return &R514783_PLUGIN;
}

LPCSTR WINAPIV GetPluginString(const DWORD /*param1*/, const DWORD /*param2*/)
{
    return "Fuck rUGP! Version 0.1.0\r\n"
        "\tThis is unofficial plugin.\r\n"
        "\t(c)2024-2025 CssXsh All rights reserved.\r\n"
        "\r\n";
}

std::wstring GetUUID(const COceanNode* const node)
{
    wchar_t buffer[MAX_PATH];
    const auto name = Unicode(node->m_pRTC ? node->m_pRTC->m_lpszClassName : nullptr, CP_SHIFT_JIS);
    swprintf(buffer, MAX_PATH, L"%s@%08X", name, node->GetAddress());
    free(name);
    return buffer;
}

std::wstring GetFilePath(const COceanNode* const node)
{
    wchar_t buffer[MAX_PATH];
    const auto format = Unicode(node->m_pRTC ? node->m_pRTC->m_lpszClassName : "bin", CP_SHIFT_JIS);
    const auto suffix = format[0] == L'C' ? format + 1 : format;
    for (auto c = suffix; *c != L'\0'; c++) *c = towlower(*c);
    swprintf(buffer, MAX_PATH, L"./%s/%08X.%s", GetGameName().c_str(), node->GetAddress(), suffix);
    free(format);
    return buffer;
}

std::wstring GetGameName()
{
    const auto command = GetCommandLineW();
    const auto l = wcsrchr(command, '\\') + 1;
    const auto r = wcschr(l, '}');
    return {l, r};
}

template <typename F>
void CALLBACK DetourAttachCallback(F& from, const F to)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&reinterpret_cast<PVOID&>(from), reinterpret_cast<PVOID>(to));
    DetourTransactionCommit();
}

template <typename F>
void CALLBACK DetourDetachCallback(F& from, const F to)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&reinterpret_cast<PVOID&>(from), reinterpret_cast<PVOID>(to));
    DetourTransactionCommit();
}

BOOL CreateMergeDirectory()
{
    const auto name = GetGameName();
    return CreateDirectoryW(name.c_str(), nullptr);
}

CVmCommand* __fastcall Merge(const CVmCommand* const ecx, cJSON* const edx) // NOLINT(*-no-recursion)
{
    const auto pClassCVmMsg = CVmMsg::GetClassCVmMsg();
    auto result = static_cast<CVmCommand*>(nullptr);
    auto prev = static_cast<CVmCommand*>(nullptr);
    auto next = static_cast<CVmCommand*>(nullptr);
    for (auto command = ecx; command != nullptr; command = command->m_pNext)
    {
        char name[MAX_PATH];
        sprintf(name, "%08X:%s", command->m_dwFlags & 0x000FFFFF, command->GetRuntimeClass()->m_lpszClassName);
        switch (*reinterpret_cast<const DWORD*>(command->GetRuntimeClass()->m_lpszClassName))
        {
        case 0x4D6D5643u: // CVmMsg
            if (command->GetRuntimeClass() != pClassCVmMsg) continue;
            {
                auto text = cJSON_GetObjectItem(edx, name);
                if (cJSON_IsString(text))
                {
                    const auto value = cJSON_GetStringValue(text, CP_GB18030);
                    const auto size = pClassCVmMsg->m_nObjectSize + ((strlen(value) + 0x04) & ~0x03);
                    const auto clone = static_cast<CVmMsg*>(malloc(size));
                    memcpy(clone, command, pClassCVmMsg->m_nObjectSize); // NOLINT(*-undefined-memory-manipulation)
                    memcpy(clone->m_area, value, (strlen(value) + 0x04) & ~0x03);
                    cJSON_free(value);
                    next = clone;
                }
                else
                {
                    const auto message = reinterpret_cast<const CVmMsg*>(command);
                    text = cJSON_CreateString(message->m_area, CP_SHIFT_JIS);
                    cJSON_AddItemToObject(edx, name, text);
                    const auto unicode = Unicode(message->m_area, CP_SHIFT_JIS);
                    const auto ansi = Ansi(unicode, CP_GB18030);
                    free(unicode);
                    const auto size = pClassCVmMsg->m_nObjectSize + ((strlen(ansi) + 0x04) & ~0x03);
                    const auto clone = static_cast<CVmMsg*>(malloc(size));
                    memcpy(clone, command, pClassCVmMsg->m_nObjectSize); // NOLINT(*-undefined-memory-manipulation)
                    memcpy(clone->m_area, ansi, (strlen(ansi) + 0x04) & ~0x03);
                    free(ansi);
                    next = clone;
                }
            }
            break;
        default:
            {
                cJSON_AddItemToObject(edx, name, cJSON_CreateNull());
                const auto size = command->GetRuntimeClass()->m_nObjectSize + command->GetVariableAreaSize();
                const auto clone = static_cast<CVmCommand*>(malloc(size));
                memcpy(clone, command, size); // NOLINT(*-undefined-memory-manipulation)
                next = clone;
            }
            break;
        }

        prev == nullptr ? (result = next) : (prev->m_pNext = next);
        prev = next;
    }

    return result;
}

CObjectProxy::CObjectProxy(const CRuntimeClass* const pClass)
{
    m_pClass = pClass;
    m_pfnCreateObject = m_pClass->m_pfnCreateObject;
    if (m_pfnCreateObject == nullptr) return;
    m_pVTBL = FindVirtualTable(m_pClass, reinterpret_cast<FARPROC>(m_pfnCreateObject));
    if (m_pVTBL == nullptr) return;
    if (strcmp(m_pClass->m_lpszClassName, "CResourceClipOnlyRegion") == 0) return;

    const auto mfc = GetMfc();
    if (pClass->IsDerivedFrom(CCommandRef::GetClassCCommandRef()))
    {
        using LPGetNextCommand = CVmCommand*(__thiscall *)(CCommandRef*);
        const auto vtbl = reinterpret_cast<const FARPROC*>(m_pVTBL);
        switch (mfc.version)
        {
        case 0x0600:
            m_pfnGetNextCommand = reinterpret_cast<LPGetNextCommand>(vtbl[0x000B]);
            break;
        case 0x0C00:
            m_pfnGetNextCommand = reinterpret_cast<LPGetNextCommand>(vtbl[0x000C]);
        default:
            // TODO ?GetNextCommand@CCommandRef@@UBEPAVCVmCommand@@XZ
            m_pfnGetNextCommand = nullptr;
            break;
        }
        //
        const auto start = reinterpret_cast<LPBYTE>(m_pVTBL->Destructor);
        for (auto offset = start; offset - start < 0x0400; offset++)
        {
            // mov     ecx, ...
            if (offset[0x00] != 0x8B) continue;
            // call    ...
            if (offset[0x02] != 0xE8) continue;
            const auto jump = *reinterpret_cast<int*>(offset + 0x03);
            const auto next = reinterpret_cast<FARPROC>(offset + 0x07 + jump);
            if (IsBadCodePtr(next)) continue;
            m_pfnDestructor = reinterpret_cast<decltype(m_pfnDestructor)>(next);
            break;
        }
    }

    if (pClass->IsDerivedFrom(CRip::GetClassCRip()))
    {
        m_pfnSerialize = reinterpret_cast<const CRio_vtbl*>(m_pVTBL)->Serialize;
    }
}

BOOL CObjectProxy::LoadFromModule(LPCSTR const lpszModuleName)
{
    const auto hModule = GetModuleHandleA(lpszModuleName);
    if (hModule == nullptr) return FALSE;
    const auto proc = GetProcAddress(hModule, "PluginThisLibrary");
    if (proc == nullptr) return FALSE;

    MODULE_MAP[hModule] = nullptr;
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&reinterpret_cast<PVOID&>(CRio::FetchLibrarySupport()), HookDrawFont);
    DetourTransactionCommit();
    auto module = reinterpret_cast<const AFX_EXTENSION_MODULE*>(proc());
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&reinterpret_cast<PVOID&>(CRio::FetchLibrarySupport()), HookDrawFont);
    DetourTransactionCommit();
    if (MODULE_MAP[hModule] != nullptr) module = MODULE_MAP[hModule];
    else MODULE_MAP[hModule] = module;
    for (auto clazz = module->pFirstSharedClass; clazz != nullptr; clazz = clazz->m_pNextClass)
    {
        const auto name = Unicode(clazz->m_lpszClassName, CP_SHIFT_JIS);
        REF_MAP[name] = new CObjectProxy(clazz);
        free(name);
    }

    return TRUE;
}

void CObjectProxy::AttachHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    for (const auto& pair : REF_MAP)
    {
        const auto ref = pair.second;
        const auto name = Unicode(ref->m_pClass->m_lpszClassName, CP_SHIFT_JIS);

        if (ref->m_pfnGetNextCommand != nullptr)
        {
            wprintf(L"DetourAttach: %s::GetNextCommand\n", name);
            DetourAttach(&reinterpret_cast<PVOID&>(ref->m_pfnGetNextCommand), HookGetNextCommand);
            wprintf(L"DetourAttach: %s::~%s\n", name, name);
            DetourAttach(&reinterpret_cast<PVOID&>(ref->m_pfnDestructor), HookDestructor);
        }

        if (ref->m_pfnSerialize != nullptr)
        {
            wprintf(L"DetourAttach: %s::Serialize\n", name);
            DetourAttach(&reinterpret_cast<PVOID&>(ref->m_pfnSerialize), HookSerialize);
        }

        free(name);
    }
    {
        wprintf(L"DetourAttach: CS5i::DrawFont\n");
        DetourAttach(&reinterpret_cast<PVOID&>(CS5i::FetchDrawFont()), HookDrawFont);
        wprintf(L"DetourAttach: CS5RFont::GetCachedFont\n");
        DetourAttach(&reinterpret_cast<PVOID&>(CS5RFont::FetchGetCachedFont()), HookGetCachedFont);
        wprintf(L"DetourAttach: CHeapHistoryPtr::Store\n");
        DetourAttach(&reinterpret_cast<PVOID&>(CMessBox::FetchStore()), HookCharacterStore);
        wprintf(L"DetourAttach: IsDBCS\n");
        DetourAttach(&reinterpret_cast<PVOID&>(CRio::FetchIsMultiple()), HookIsMultiple);
    }
    DetourTransactionCommit();

    AttachCharacterPatch("UnivUI");
    AttachCharacterPatch("rvmm");
    AttachCharacterPatch("Vm60");
}

void CObjectProxy::DetachHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    for (const auto& pair : REF_MAP)
    {
        const auto ref = pair.second;
        const auto name = Unicode(ref->m_pClass->m_lpszClassName, CP_SHIFT_JIS);

        if (ref->m_pfnGetNextCommand != nullptr)
        {
            wprintf(L"DetourDetach: %s::GetNextCommand\n", name);
            DetourDetach(&reinterpret_cast<PVOID&>(ref->m_pfnGetNextCommand), HookGetNextCommand);
            wprintf(L"DetourDetach: %s::~%s\n", name, name);
            DetourDetach(&reinterpret_cast<PVOID&>(ref->m_pfnDestructor), HookDestructor);
        }

        if (ref->m_pfnSerialize != nullptr)
        {
            wprintf(L"DetourDetach: %s::Serialize\n", name);
            DetourDetach(&reinterpret_cast<PVOID&>(ref->m_pfnSerialize), HookSerialize);
        }

        free(name);
    }
    {
        wprintf(L"DetourDetach: CS5i::DrawFont\n");
        DetourDetach(&reinterpret_cast<PVOID&>(CS5i::FetchDrawFont()), HookDrawFont);
        wprintf(L"DetourDetach: CS5RFont::GetCachedFont\n");
        DetourDetach(&reinterpret_cast<PVOID&>(CS5RFont::FetchGetCachedFont()), HookGetCachedFont);
        wprintf(L"DetourDetach: CHeapHistoryPtr::Store\n");
        DetourDetach(&reinterpret_cast<PVOID&>(CMessBox::FetchStore()), HookCharacterStore);
        wprintf(L"DetourDetach: IsDBCS\n");
        DetourDetach(&reinterpret_cast<PVOID&>(CRio::FetchIsMultiple()), HookIsMultiple);
    }
    DetourTransactionCommit();

    DetachCharacterPatch("UnivUI");
    DetachCharacterPatch("rvmm");
    DetachCharacterPatch("Vm60");
}

void CObjectProxy::Clear()
{
    for (auto& pair : COMMAND_MAP)
    {
        auto p = pair.second;
        pair.second = nullptr;
        while (p != nullptr)
        {
            const auto command = p;
            p = p->m_pNext;
            free(command);
        }
    }
    COMMAND_MAP.clear();

    for (auto& pair : REF_MAP)
    {
        const auto p = pair.second;
        pair.second = nullptr;
        delete p;
    }
    REF_MAP.clear();
}

const CObject_vtbl* CObjectProxy::FindVirtualTable( // NOLINT(*-no-recursion)
    const CRuntimeClass* const rtc, FARPROC const ctor) // NOLINT(*-misplaced-const)
{
    if (IsBadCodePtr(ctor)) return nullptr;
    const auto module = DetourGetContainingModule(const_cast<CRuntimeClass*>(rtc));
    if (DetourGetContainingModule(ctor) != module) return nullptr;
    const auto start = reinterpret_cast<LPBYTE>(ctor);
    for (auto offset = start; offset - start < 0x0400; offset++)
    {
        // mov     dword ptr [*], ...
        if (offset[0x00] != 0xC7) continue;
        const auto address = *reinterpret_cast<const CObject_vtbl* const*>(offset + 0x02);
        if (IsBadReadPtr(address, sizeof(CObject_vtbl))) continue;
        if (IsBadCodePtr(reinterpret_cast<FARPROC>(address->GetRuntimeClass))) continue;
        const auto get = reinterpret_cast<DWORD>(address->GetRuntimeClass);
        // mov     eax, ...
        const auto clazz = *reinterpret_cast<const CRuntimeClass* const*>(get + 0x01);
        if (rtc == clazz) return address;
    }
    if (ctor != reinterpret_cast<FARPROC>(rtc->m_pfnCreateObject)) return nullptr;
    for (auto offset = start; offset - start < 0x0400; offset++)
    {
        // mov     ecx, ...
        if (offset[0x00] != 0x8B) continue;
        // call    ...
        if (offset[0x02] != 0xE8) continue;
        const auto jump = *reinterpret_cast<const INT*>(offset + 0x03);
        const auto next = reinterpret_cast<FARPROC>(offset + 0x07 + jump);
        const auto vtbl = FindVirtualTable(rtc, next);
        if (vtbl != nullptr) return vtbl;
    }

    return nullptr;
}

void CObjectProxy::AttachCharacterPatch(LPCSTR const lpszModuleName)
{
    const auto hModule = GetModuleHandleA(lpszModuleName);
    if (hModule == nullptr) return;
    const auto size = DetourGetModuleSize(hModule);
    const auto start = reinterpret_cast<DWORD>(hModule);
    for (auto offset = start; offset < start + size; offset++)
    {
        if (IsBadReadPtr(reinterpret_cast<FARPROC>(offset), sizeof(DWORD))) continue;
        switch (*reinterpret_cast<const DWORD*>(offset))
        {
        // add     al, 5Fh
        // cmp     al, 3Bh
        case 0x3B3C5F04u:
            {
                const auto address = reinterpret_cast<LPBYTE>(offset + 0x00);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + 0x04));
                record->block_size = 0x04;
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                address[0x00] = 0x34u;
                address[0x01] = 0xA0u;
                address[0x03] = 0x7Fu;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
                AttachCharacterHandle(address + record->block_size);
            }
            break;
        // add     bl, 5Fh
        // cmp     bl, 3Bh
        case 0x3BFB805Fu:
        // add     cl, 5Fh
        // cmp     cl, 3Bh
        case 0x3BF9805Fu:
        // add     dl, 5Fh
        // cmp     dl, 3Bh
        case 0x3BFA805Fu:
            {
                const auto address = reinterpret_cast<LPBYTE>(offset - 0x02);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + 0x06));
                record->block_size = 0x06;
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                address[0x01] = address[0x01] & 0x0Fu | 0xF0u;
                address[0x02] = 0xA0u;
                address[0x05] = 0x7Fu;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
                // AttachCharacterHandle(address + record->block_size);
            }
            break;
        // push    esi
        // call    ds:__imp_?IsDBCS@@YAHD@Z
        // add     esp, 4
        // test    eax, eax
        case 0x8504C483u:
            {
                const auto is = GetProcAddress(GetModuleHandleA("GMfc"), "?IsDBCS@@YAHD@Z");
                if (is == nullptr) continue;
                const auto address = reinterpret_cast<LPBYTE>(offset - 0x07);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                if (address[0x01] != 0xFFu || address[0x02] != 0x15u) continue;
                if (IsBadCodePtr(*reinterpret_cast<FARPROC*>(address + 0x03))) continue;
                const auto proc = **reinterpret_cast<FARPROC**>(address + 0x03);
                if (proc != is) continue;
                // AttachCharacterHandle(address + 0x0C);
            }
            break;
        // 0xA1A1
        case 0x00008140u:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A1A1u;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA1B8
        case 0x00008175u:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A1B8u;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA1B9
        case 0x00008176u:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A1B9u;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA1BE
        case 0x00008179u:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A1BEu;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA1BF
        case 0x0000817Au:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A1BFu;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA844
        case 0x0000815Cu:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A844u;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        // 0xA9A4
        case 0x0000849Fu:
            {
                const auto address = reinterpret_cast<LPDWORD>(offset);
                if (IsBadCodePtr(reinterpret_cast<FARPROC>(address))) continue;
                const auto record = static_cast<CodePatchRecord*>(malloc(sizeof(CodePatchRecord) + sizeof(DWORD)));
                record->block_size = sizeof(DWORD);
                memcpy(record->origin, address, record->block_size);
                auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
                VirtualProtect(address, record->block_size, protect, &protect);
                *address = 0x0000A9A4u;
                VirtualProtect(address, record->block_size, protect, &protect);
                PATCH_CACHE[address] = record;
            }
            break;
        default:
            break;
        }
    }
}

void CObjectProxy::DetachCharacterPatch(LPCSTR const lpszModuleName)
{
    const auto hModule = GetModuleHandleA(lpszModuleName);
    if (hModule == nullptr) return;
    for (auto& pair : PATCH_CACHE)
    {
        if (DetourGetContainingModule(pair.first) != hModule) continue;
        const auto address = pair.first;
        const auto record = pair.second;
        pair.second = nullptr;

        auto protect = static_cast<DWORD>(PAGE_EXECUTE_READWRITE);
        VirtualProtect(address, record->block_size, protect, &protect);
        memcpy(address, record->origin, record->block_size);
        VirtualProtect(address, record->block_size, protect, &protect);
        free(record);
    }
}

void CObjectProxy::AttachCharacterHandle(LPBYTE const address) // NOLINT(*-misplaced-const)
{
    const auto module = DetourGetContainingModule(address);
    wchar_t name[MAX_PATH];
    GetModuleFileNameW(module, name, MAX_PATH);
    auto start = static_cast<LPBYTE>(nullptr);
    auto end = static_cast<LPBYTE>(nullptr);
    switch (address[0x00])
    {
    // ja      ...
    case 0x0Fu:
        start = address + 0x06;
        end = start + *reinterpret_cast<int*>(address + 0x02);
        break;
    // jz      short ...
    case 0x74u:
    // jbe     short ...
    case 0x76u:
    // ja      short ...
    case 0x77u:
        start = address + 0x02;
        end = start + *reinterpret_cast<char*>(address + 0x01);
        break;
    default:
        wprintf(L"AttachCharacterHandle Error: 0x%p at %s\n", address, name);
        return;
    }
    // jmp     short ...
    if (start[0x01] == 0xEBu) end = start + 0x01;
    else if (start[0x05] == 0xEBu) end = start + 0x05;
    // jz      ...
    if (start[0x08] == 0x74u) end = start + 0x08;
    // cmp     ..., ...
    for (auto offset = start; offset < end; offset++)
    {
        if (offset[0x00] != 0x81u) continue;
        if (offset[0x04] != 0x00u) continue;
        if (offset[0x05] != 0x00u) continue;
        end = offset;
        break;
    }
    // mov     [esp+...], ...
    if (end - start == 0x0E && start[0x0A] == 0x89u)
    {
        end = start + 0x0A;
    }
    // inc     ...
    if (start[0x00] == 0x0Fu && start[0x03] != 0xC1u)
    {
        end = start + 0x04;
    }
    // test     ...
    if (start[0x00] == 0x85u)
    {
        end = start + 0x00;
    }

    switch (const auto size = end - start)
    {
    case 0x00u:
        break;
    // inc     ...
    case 0x01u:
        {
            const auto codes = static_cast<LPBYTE>(VirtualAlloc(
                nullptr,
                0x0020,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));
            memset(codes, 0xCCu, 0x0020);
            auto hook = end;
            const auto diff = start[0x00] - 0x40u;
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&reinterpret_cast<PVOID&>(hook), codes);
            DetourTransactionCommit();
            auto index = 0x00;
            // dec     ...
            codes[index++] = 0x48 + diff;
            // dec     ...
            codes[index++] = 0x48 + diff;
            // push    eax
            codes[index++] = 0x50;
            // push    ...
            codes[index++] = 0x50 + diff;
            // call    ...
            codes[index++] = 0xE8;
            *reinterpret_cast<int*>(codes + index) =
                reinterpret_cast<int>(HookCharacterByteSize) - reinterpret_cast<int>(codes + index + 0x04);
            index += 0x04;
            // add     ..., eax
            codes[index++] = 0x01, codes[index++] = 0xC0 + diff;
            // pop     eax
            codes[index++] = 0x58;
            // jmp     ...
            codes[index++] = 0xE9;
            *reinterpret_cast<int*>(codes + index) =
                reinterpret_cast<int>(hook) - reinterpret_cast<int>(codes + index + 0x04);
        }
        return;
    // movzx   ..., byte ptr [...]
    // inc     ...
    case 0x04:
        break;
    // mov     .., [...]
    // mov     [...], ..
    // inc     ...
    case 0x05:
        break;
    // mov     .., [...+1]
    // mov     .., [...+1]
    // cmp     .., ..
    case 0x08:
        break;
    // movzx   ..., byte ptr [...]
    // shl     ..., 8
    // or      ..., ...
    // inc     ...
    case 0x09:
    // xor     ..., ...
    // mov     .., [...]
    // shl     ..., 8
    // or      ..., ...
    // inc     ...
    case 0x0A:
    // xor     ..., ...
    // mov     .., [...+0]
    // shl     ..., 8
    // or      ..., ...
    // inc     ...
    case 0x0B:
        if (end[-0x04] != 0x08u) return;
        {
            const auto codes = static_cast<LPBYTE>(VirtualAlloc(
                nullptr,
                0x0040,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));
            memset(codes, 0xCCu, 0x0040);
            auto hook = end;
            const auto diff = end[-0x01] - 0x40u;
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&reinterpret_cast<PVOID&>(hook), codes);
            DetourTransactionCommit();
            auto index = 0x00;
            // dec     ...
            codes[index++] = 0x48 + diff;
            // dec     ...
            codes[index++] = 0x48 + diff;
            // shr     ..., 0x10
            codes[index++] = 0xC1, codes[index++] = end[-0x05], codes[index++] = 0x10;
            // push    eax
            codes[index++] = 0x50;
            // push    ...
            codes[index++] = 0x50 + diff;
            // call    ...
            codes[index++] = 0xE8;
            *reinterpret_cast<int*>(codes + index) =
                reinterpret_cast<int>(HookCharacterByteSize) - reinterpret_cast<int>(codes + index + 0x04);
            index += 0x04;
            const auto loop = index;
            // push    eax
            codes[index++] = 0x50;
            // action
            memcpy(codes + index, start, size);
            index += size;
            // pop     eax
            codes[index++] = 0x58;
            // dec     eax
            codes[index++] = 0x48;
            // test    eax, eax
            codes[index++] = 0x85, codes[index++] = 0xC0;
            // jnz     loop
            codes[index++] = 0x0F, codes[index++] = 0x85;
            *reinterpret_cast<int*>(codes + index) =
                reinterpret_cast<int>(codes + loop) - reinterpret_cast<int>(codes + index + 0x04);
            index += 0x04;
            // pop     eax
            codes[index++] = 0x58;
            // jmp     ...
            codes[index++] = 0xE9;
            *reinterpret_cast<int*>(codes + index) =
                reinterpret_cast<int>(hook) - reinterpret_cast<int>(codes + index + 0x04);
        }
        return;
    default:
        break;
    }
    wprintf(L"AttachCharacterHandle 0x%p ~ 0x%p diff 0x%02X at %s\n", start, end, end - start, name);
}

void CObjectProxy::HookSupportRio(AFX_EXTENSION_MODULE& module)
{
    CRio::FetchLibrarySupport()(module);
    MODULE_MAP[DetourGetContainingModule(&module)] = &module;
}

void CObjectProxy::HookDestructor(CRio* const ecx)
{
    const auto uuid = GetUUID(ecx->m_pNode);
    const auto name = Unicode(ecx->GetRuntimeClass()->m_lpszClassName, CP_SHIFT_JIS);
    const auto ref = REF_MAP[name];
    wprintf(L"Hook %s::~%s(this=%s)\n", name, name, uuid.c_str());
    free(name);

    auto cache = COMMAND_MAP[uuid];
    COMMAND_MAP[uuid] = nullptr;
    while (cache != nullptr)
    {
        const auto command = cache;
        cache = cache->m_pNext;
        free(command);
    }

    return ref->m_pfnDestructor(ecx);
}

void CObjectProxy::HookSerialize(CRio* const ecx, CPmArchive* const archive)
{
    const auto uuid = GetUUID(ecx->m_pNode);
    const auto name = Unicode(ecx->GetRuntimeClass()->m_lpszClassName, CP_SHIFT_JIS);
    const auto ref = REF_MAP[name];
    wprintf(L"Hook %s::Serialize(this=%s)\n", name, uuid.c_str());
    free(name);
    if (ecx->m_pNode->m_dwResAddr == 0x00000000) return ref->m_pfnSerialize(ecx, archive);

    const auto path = GetFilePath(ecx->m_pNode);
    const auto hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        const auto ansi = Ansi(path.c_str(), CP_ACP);
        const auto load = CPmArchive::CreateLoadFilePmArchive(ansi);
        free(ansi);
        ref->m_pfnSerialize(ecx, load);
        CPmArchive::DestroyPmArchive(load);
    }
    else
    {
        ref->m_pfnSerialize(ecx, archive);
        const auto ansi = Ansi(path.c_str(), CP_ACP);
        const auto save = CPmArchive::CreateSaveFilePmArchive(ansi);
        free(ansi);
        ref->m_pfnSerialize(ecx, save);
        CPmArchive::DestroyPmArchive(save);
    }
}

CVmCommand* CObjectProxy::HookGetNextCommand(CCommandRef* const ecx)
{
    const auto uuid = GetUUID(ecx->m_pNode);
    const auto cache = COMMAND_MAP[uuid];
    if (cache != nullptr) return cache;
    const auto name = Unicode(ecx->GetRuntimeClass()->m_lpszClassName, CP_SHIFT_JIS);
    const auto ref = REF_MAP[name];
    wprintf(L"Hook %s::GetNextCommand(this=%s)\n", name, uuid.c_str());
    free(name);
    auto value = ref->m_pfnGetNextCommand(ecx);

    const auto path = GetFilePath(ecx->m_pNode) + L".json";
    const auto hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return value;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        const auto size = GetFileSize(hFile, nullptr);
        const auto buffer = static_cast<char*>(malloc(size));
        ReadFile(hFile, buffer, size, nullptr, nullptr);
        CloseHandle(hFile);
        const auto patch = cJSON_ParseWithLength(buffer, size);
        value = Merge(value, patch);
        free(buffer);
        cJSON_free(patch);
    }
    else
    {
        const auto patch = cJSON_CreateObject();
        value = Merge(value, patch);
        const auto buffer = cJSON_Print(patch);
        WriteFile(hFile, buffer, strlen(buffer), nullptr, nullptr);
        CloseHandle(hFile);
        cJSON_free(buffer);
        cJSON_free(patch);
    }

    COMMAND_MAP[uuid] = value;

    return value;
}

BOOL CObjectProxy::HookIsMultiple(char const c)
{
    return (c & 0x80u) == 0x80u;
}

int CObjectProxy::HookDrawFont(
    LPVOID const ecx, DWORD const x, DWORD const y, WORD* const rect, WORD* const out, // NOLINT(*-misplaced-const)
    UINT uChar, CFontContext* const context)
{
    if ((uChar & 0xFF00) != 0x0000 && (uChar & 0x00FF) <= 0x0039) uChar = CHARACTER_MAP[uChar];
    return CS5i::FetchDrawFont()(ecx, x, y, rect, out, uChar, context);
}

LPVOID CObjectProxy::HookGetCachedFont(CS5RFont* ecx, UINT const uChar, COceanNode* const node)
{
    if (uChar <= 0xFFFF) return ecx->GetCachedFont(uChar, node);
    auto& font = FONT_CACHE[uChar];
    if (font == nullptr) font = ecx->CreateNewFont(uChar, node);
    return font;
}

void CObjectProxy::HookCharacterStore(
    LPVOID const ecx, LPCVOID const source, SIZE_T const size) // NOLINT(*-misplaced-const)
{
    CMessBox::FetchStore()(ecx, source, size);
    if (size != 2) return;
    const auto key = *static_cast<const WORD*>(source);
    if ((key & 0xFF00) == 0x0000 || (key & 0x00FF) >= 0x0040) return;
    CHARACTER_MAP[key] = *static_cast<const UINT*>(source);
}

void CObjectProxy::HookCharacterLoad(
    LPVOID const ecx, LPVOID const target, SIZE_T const size) // NOLINT(*-misplaced-const)
{
    CMessBox::FetchLoad()(ecx, target, size);
}

int CObjectProxy::HookCharacterByteSize(
    CRio* const ecx, LPCSTR const text)
{
    // GB18030
    if ((text[0x0000] & 0x80u) == 0x00u) return 1;
    if ((text[0x0001] & 0xC0u) == 0x00u) return 4;
    return 2;
}

std::map<std::wstring, CObjectProxy*> CObjectProxy::REF_MAP;

std::map<std::wstring, CVmCommand*> CObjectProxy::COMMAND_MAP;

std::map<HMODULE, const AFX_EXTENSION_MODULE*> CObjectProxy::MODULE_MAP;

std::map<LPVOID, CodePatchRecord*> CObjectProxy::PATCH_CACHE;

std::map<WORD, UINT> CObjectProxy::CHARACTER_MAP;

std::map<UINT, LPVOID> CObjectProxy::FONT_CACHE;

void COceanTree::AttachHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    wprintf(L"DetourAttach: GetMotherOcean\n");
    DetourAttach(&reinterpret_cast<PVOID&>(COceanNode::FetchGetMotherOcean()), HookGetMotherOcean);
    DetourTransactionCommit();
}

void COceanTree::DetachHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    wprintf(L"DetourDetach: GetMotherOcean\n");
    DetourDetach(&reinterpret_cast<PVOID&>(COceanNode::FetchGetMotherOcean()), HookGetMotherOcean);
    DetourTransactionCommit();
}

COceanNode** COceanTree::HookGetMotherOcean(COceanNode** const pNode)
{
    wprintf(L"Hook GetMotherOcean()\n");
    *pNode = *COceanNode::FetchGetMotherOcean()(pNode);
    const auto root = *pNode;
    auto iterator = Iterator(root);
    for (auto node = iterator.Next(); node != nullptr; node = iterator.Next())
    {
        // TODO ...
    }

    return pNode;
}

COceanTree::Iterator::Iterator(const COceanNode* const root)
{
    m_pNode = root;
    m_nLevel = root ? 1 : 0;
}

DWORD COceanTree::Iterator::Level() const
{
    return m_nLevel;
}

const COceanNode* COceanTree::Iterator::Next()
{
    const auto node = m_pNode;
    if (node == nullptr) return nullptr;
    constexpr auto empty = COceanNode::Children();
    const auto null = COceanNode::GetNull();

    for (auto root = m_pNode; root && m_nLevel; root = root->m_pParent, m_nLevel--)
    {
        auto mask = static_cast<WORD>(0x0000);
        const auto visited = m_pVisited[root];

        mask = static_cast<WORD>(0x0001);
        for (const auto child : (root->m_pChildren ? root->m_pChildren : &empty)->m_pBucket)
        {
            if (child == nullptr) continue;
            if (child == null) continue;
            mask <<= 0x01;
            if (visited & mask) continue;
            m_pNode = child;
            m_nLevel++;
            m_pVisited[root] = visited | mask;

            return node;
        }

        if (root->m_pNext == nullptr) continue;
        if (root->m_pNext == null) continue;
        mask = static_cast<WORD>(0x8000);
        if (visited & mask) continue;
        m_pNode = root->m_pNext;
        m_pVisited[root] = visited | mask;

        return node;
    }

    m_pNode = nullptr;
    m_nLevel = 0;
    return nullptr;
}
