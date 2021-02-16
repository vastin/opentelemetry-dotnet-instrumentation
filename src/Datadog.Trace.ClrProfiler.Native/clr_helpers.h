#ifndef DD_CLR_PROFILER_CLR_HELPERS_H_
#define DD_CLR_PROFILER_CLR_HELPERS_H_

#include <corhlpr.h>
#include <corprof.h>
#include <functional>
#include <utility>

#include "com_ptr.h"
#include "integration.h"
#include <set>
#include "util.h"

namespace trace {
class ModuleMetadata;

const size_t kNameMaxSize = 1024;
const ULONG kEnumeratorMax = 256;

const auto SystemBoolean = "System.Boolean"_W;
const auto SystemChar = "System.Char"_W;
const auto SystemByte = "System.Byte"_W;
const auto SystemSByte = "System.SByte"_W;
const auto SystemUInt16 = "System.UInt16"_W;
const auto SystemInt16 = "System.Int16"_W;
const auto SystemInt32 = "System.Int32"_W;
const auto SystemUInt32 = "System.UInt32"_W;
const auto SystemInt64 = "System.Int64"_W;
const auto SystemUInt64 = "System.UInt64"_W;
const auto SystemSingle = "System.Single"_W;
const auto SystemDouble = "System.Double"_W;
const auto SystemIntPtr = "System.IntPtr"_W;
const auto SystemUIntPtr = "System.UIntPtr"_W;
const auto SystemString = "System.String"_W;
const auto SystemObject = "System.Object"_W;
const auto SystemException = "System.Exception"_W;
const auto SystemTypeName = "System.Type"_W;
const auto GetTypeFromHandleMethodName = "GetTypeFromHandle"_W;
const auto RuntimeTypeHandleTypeName = "System.RuntimeTypeHandle"_W;
const auto SystemReflectionMethodBaseName = "System.Reflection.MethodBase"_W;
const auto GetMethodFromHandleMethodName = "GetMethodFromHandle"_W;
const auto RuntimeMethodHandleTypeName = "System.RuntimeMethodHandle"_W;

template <typename T>
class EnumeratorIterator;

template <typename T>
class Enumerator {
 private:
  const std::function<HRESULT(HCORENUM*, T[], ULONG, ULONG*)> callback_;
  const std::function<void(HCORENUM)> close_;
  mutable HCORENUM ptr_;

 public:
  Enumerator(std::function<HRESULT(HCORENUM*, T[], ULONG, ULONG*)> callback,
             std::function<void(HCORENUM)> close)
      : callback_(callback), close_(close), ptr_(nullptr) {}

  Enumerator(const Enumerator& other) = default;

  Enumerator& operator=(const Enumerator& other) = default;

  ~Enumerator() { close_(ptr_); }

  EnumeratorIterator<T> begin() const {
    return EnumeratorIterator<T>(this, S_OK);
  }

  EnumeratorIterator<T> end() const {
    return EnumeratorIterator<T>(this, S_FALSE);
  }

  HRESULT Next(T arr[], ULONG max, ULONG* cnt) const {
    return callback_(&ptr_, arr, max, cnt);
  }
};

template <typename T>
class EnumeratorIterator {
 private:
  const Enumerator<T>* enumerator_;
  HRESULT status_ = S_FALSE;
  T arr_[kEnumeratorMax]{};
  ULONG idx_ = 0;
  ULONG sz_ = 0;

 public:
  EnumeratorIterator(const Enumerator<T>* enumerator, HRESULT status)
      : enumerator_(enumerator) {
    if (status == S_OK) {
      status_ = enumerator_->Next(arr_, kEnumeratorMax, &sz_);
      if (status_ == S_OK && sz_ == 0) {
        status_ = S_FALSE;
      }
    } else {
      status_ = status;
    }
  }

  bool operator!=(EnumeratorIterator const& other) const {
    return enumerator_ != other.enumerator_ ||
           (status_ == S_OK) != (other.status_ == S_OK);
  }

  T const& operator*() const { return arr_[idx_]; }

  EnumeratorIterator<T>& operator++() {
    if (idx_ < sz_ - 1) {
      idx_++;
    } else {
      idx_ = 0;
      status_ = enumerator_->Next(arr_, kEnumeratorMax, &sz_);
      if (status_ == S_OK && sz_ == 0) {
        status_ = S_FALSE;
      }
    }
    return *this;
  }
};

static Enumerator<mdTypeDef> EnumTypeDefs(
    const ComPtr<IMetaDataImport2>& metadata_import) {
  return Enumerator<mdTypeDef>(
      [metadata_import](HCORENUM* ptr, mdTypeDef arr[], ULONG max,
                        ULONG* cnt) -> HRESULT {
        return metadata_import->EnumTypeDefs(ptr, arr, max, cnt);
      },
      [metadata_import](HCORENUM ptr) -> void {
        metadata_import->CloseEnum(ptr);
      });
}

static Enumerator<mdTypeRef> EnumTypeRefs(
    const ComPtr<IMetaDataImport2>& metadata_import) {
  return Enumerator<mdTypeRef>(
      [metadata_import](HCORENUM* ptr, mdTypeRef arr[], ULONG max,
                        ULONG* cnt) -> HRESULT {
        return metadata_import->EnumTypeRefs(ptr, arr, max, cnt);
      },
      [metadata_import](HCORENUM ptr) -> void {
        metadata_import->CloseEnum(ptr);
      });
}

static Enumerator<mdMethodDef> EnumMethods(
    const ComPtr<IMetaDataImport2>& metadata_import,
    const mdToken& parent_token) {
  return Enumerator<mdMethodDef>(
      [metadata_import, parent_token](HCORENUM* ptr, mdMethodDef arr[],
                                      ULONG max, ULONG* cnt) -> HRESULT {
        return metadata_import->EnumMethods(ptr, parent_token, arr, max, cnt);
      },
      [metadata_import](HCORENUM ptr) -> void {
        metadata_import->CloseEnum(ptr);
      });
}

static Enumerator<mdMemberRef> EnumMemberRefs(
    const ComPtr<IMetaDataImport2>& metadata_import,
    const mdToken& parent_token) {
  return Enumerator<mdMemberRef>(
      [metadata_import, parent_token](HCORENUM* ptr, mdMemberRef arr[],
                                      ULONG max, ULONG* cnt) -> HRESULT {
        return metadata_import->EnumMemberRefs(ptr, parent_token, arr, max,
                                               cnt);
      },
      [metadata_import](HCORENUM ptr) -> void {
        metadata_import->CloseEnum(ptr);
      });
}

static Enumerator<mdModuleRef> EnumModuleRefs(
    const ComPtr<IMetaDataImport2>& metadata_import) {
  return Enumerator<mdModuleRef>(
      [metadata_import](HCORENUM* ptr, mdModuleRef arr[], ULONG max,
                        ULONG* cnt) -> HRESULT {
        return metadata_import->EnumModuleRefs(ptr, arr, max, cnt);
      },
      [metadata_import](HCORENUM ptr) -> void {
        metadata_import->CloseEnum(ptr);
      });
}

static Enumerator<mdAssemblyRef> EnumAssemblyRefs(
    const ComPtr<IMetaDataAssemblyImport>& assembly_import) {
  return Enumerator<mdAssemblyRef>(
      [assembly_import](HCORENUM* ptr, mdAssemblyRef arr[], ULONG max,
                        ULONG* cnt) -> HRESULT {
        return assembly_import->EnumAssemblyRefs(ptr, arr, max, cnt);
      },
      [assembly_import](HCORENUM ptr) -> void {
        assembly_import->CloseEnum(ptr);
      });
}

struct RuntimeInformation {
  COR_PRF_RUNTIME_TYPE runtime_type;
  USHORT major_version;
  USHORT minor_version;
  USHORT build_version;
  USHORT qfe_version;

  RuntimeInformation() : runtime_type((COR_PRF_RUNTIME_TYPE)0x0), major_version(0), minor_version(0), build_version(0), qfe_version(0) {}

  RuntimeInformation(COR_PRF_RUNTIME_TYPE runtime_type, USHORT major_version, USHORT minor_version, USHORT build_version, USHORT qfe_version)
    : runtime_type(runtime_type),
      major_version(major_version),
      minor_version(minor_version),
      build_version(build_version),
      qfe_version(qfe_version) {}

  RuntimeInformation& operator=(const RuntimeInformation& other) {
    runtime_type = other.runtime_type;
    major_version = other.major_version;
    minor_version = other.minor_version;
    build_version = other.build_version;
    qfe_version = other.qfe_version;
    return *this;
  }

  bool is_desktop() const { return runtime_type == COR_PRF_DESKTOP_CLR; }
  bool is_core() const { return runtime_type == COR_PRF_CORE_CLR; }
};

struct AssemblyInfo {
  const AssemblyID id;
  const WSTRING name;
  const ModuleID manifest_module_id;
  const AppDomainID app_domain_id;
  const WSTRING app_domain_name;

  AssemblyInfo() : id(0), name(""_W), manifest_module_id(0), app_domain_id(0), app_domain_name(""_W) {}

  AssemblyInfo(AssemblyID id, WSTRING name, ModuleID manifest_module_id, AppDomainID app_domain_id,
               WSTRING app_domain_name)
      : id(id),
        name(name),
        manifest_module_id(manifest_module_id),
        app_domain_id(app_domain_id),
        app_domain_name(app_domain_name) {}

  bool IsValid() const { return id != 0; }
};

struct AssemblyMetadata {
  const ModuleID module_id;
  const WSTRING name;
  const mdAssembly assembly_token;
  const Version version;

  AssemblyMetadata() : module_id(0), name(""_W), assembly_token(mdTokenNil) {}

  AssemblyMetadata(ModuleID module_id, WSTRING name, mdAssembly assembly_token,
                   USHORT major, USHORT minor, USHORT build, USHORT revision)
      : module_id(module_id),
        name(name),
        assembly_token(assembly_token),
        version(Version(major, minor, build, revision)) {}

  bool IsValid() const { return module_id != 0; }
};

struct AssemblyProperty {
  const void* ppbPublicKey;
  ULONG pcbPublicKey;
  ULONG pulHashAlgId;
  ASSEMBLYMETADATA pMetaData{};
  WSTRING szName;
  DWORD assemblyFlags = 0;

  AssemblyProperty()
      : ppbPublicKey(nullptr), pcbPublicKey(0), pulHashAlgId(0), szName(""_W) {}
};

struct ModuleInfo {
  const ModuleID id;
  const WSTRING path;
  const AssemblyInfo assembly;
  const DWORD flags;

  ModuleInfo() : id(0), path(""_W), assembly({}), flags(0) {}
  ModuleInfo(ModuleID id, WSTRING path, AssemblyInfo assembly, DWORD flags)
      : id(id), path(path), assembly(assembly), flags(flags) {}

  bool IsValid() const { return id != 0; }

  bool IsWindowsRuntime() const {
    return ((flags & COR_PRF_MODULE_WINDOWS_RUNTIME) != 0);
  }
};

struct TypeInfo {
  const mdToken id;
  const WSTRING name;
  const mdTypeSpec type_spec;
  const ULONG32 token_type;
  const TypeInfo* extend_from;
  const bool valueType;
  const bool isGeneric;

  TypeInfo()
      : id(0),
        name(""_W),
        type_spec(0),
        token_type(0),
        extend_from(nullptr),
        valueType(false),
        isGeneric(false) {}
  TypeInfo(mdToken id, WSTRING name, mdTypeSpec type_spec, ULONG32 token_type,
           const TypeInfo* extend_from, bool valueType, bool isGeneric) 
      : id(id),
        name(name),
        type_spec(type_spec),
        token_type(token_type),
        extend_from(extend_from),
        valueType(valueType),
        isGeneric(isGeneric) {}

  bool IsValid() const { return id != 0; }
};

enum MethodArgumentTypeFlag {
  TypeFlagByRef = 0x01,
  TypeFlagVoid = 0x02,
  TypeFlagBoxedType = 0x04
};

struct FunctionMethodArgument {
  ULONG offset;
  ULONG length;
  PCCOR_SIGNATURE pbBase;
  mdToken GetTypeTok(ComPtr<IMetaDataEmit2>& pEmit,
                     mdAssemblyRef corLibRef) const;
  WSTRING GetTypeTokName(ComPtr<IMetaDataImport2>& pImport) const;
  int GetTypeFlags(unsigned& elementType) const;
  ULONG GetSignature(PCCOR_SIGNATURE& data) const;
};

struct FunctionMethodSignature {
 private:
  PCCOR_SIGNATURE pbBase;
  unsigned len;
  ULONG numberOfTypeArguments = 0;
  ULONG numberOfArguments = 0;
  FunctionMethodArgument ret{};
  std::vector<FunctionMethodArgument> params;

 public:
  FunctionMethodSignature() : pbBase(nullptr), len(0) {}
  FunctionMethodSignature(PCCOR_SIGNATURE pb, unsigned cbBuffer) {
    pbBase = pb;
    len = cbBuffer;
  };
  ULONG NumberOfTypeArguments() const { return numberOfTypeArguments; }
  ULONG NumberOfArguments() const { return numberOfArguments; }
  WSTRING str() const { return HexStr(pbBase, len); }
  FunctionMethodArgument GetRet() const { return ret; }
  std::vector<FunctionMethodArgument> GetMethodArguments() const { return params; }
  HRESULT TryParse();
  bool operator==(const FunctionMethodSignature& other) const {
    return memcmp(pbBase, other.pbBase, len);
  }
  CorCallingConvention CallingConvention() const {
    return CorCallingConvention(len == 0 ? 0 : pbBase[0]);
  }
  bool IsEmpty() const { return len == 0; }
};


struct FunctionInfo {
  const mdToken id;
  const WSTRING name;
  const TypeInfo type;
  const BOOL is_generic;
  const MethodSignature signature;
  const MethodSignature function_spec_signature;
  const mdToken method_def_id;
  FunctionMethodSignature method_signature;

  FunctionInfo()
      : id(0), name(""_W), type({}), is_generic(false), method_def_id(0), method_signature({}) {}

  FunctionInfo(mdToken id, WSTRING name, TypeInfo type,
               MethodSignature signature,
               MethodSignature function_spec_signature, 
               mdToken method_def_id,
               FunctionMethodSignature method_signature)
      : id(id),
        name(name),
        type(type),
        is_generic(true),
        signature(signature),
        function_spec_signature(function_spec_signature),
        method_def_id(method_def_id),
        method_signature(method_signature) {}

  FunctionInfo(mdToken id, WSTRING name, TypeInfo type,
               MethodSignature signature,
               FunctionMethodSignature method_signature)
      : id(id),
        name(name),
        type(type),
        is_generic(false),
        signature(signature),
        method_def_id(0),
        method_signature(method_signature) {}

  bool IsValid() const { return id != 0; }
};

RuntimeInformation GetRuntimeInformation(ICorProfilerInfo4* info);

AssemblyInfo GetAssemblyInfo(ICorProfilerInfo4* info,
                             const AssemblyID& assembly_id);

AssemblyMetadata GetAssemblyMetadata(
    const ModuleID& module_id,
    const ComPtr<IMetaDataAssemblyImport>& assembly_import);

AssemblyMetadata GetAssemblyImportMetadata(
    const ComPtr<IMetaDataAssemblyImport>& assembly_import);

AssemblyMetadata GetReferencedAssemblyMetadata(
    const ComPtr<IMetaDataAssemblyImport>& assembly_import,
    const mdAssemblyRef& assembly_ref);

FunctionInfo GetFunctionInfo(const ComPtr<IMetaDataImport2>& metadata_import,
                             const mdToken& token);

ModuleInfo GetModuleInfo(ICorProfilerInfo4* info, const ModuleID& module_id);

TypeInfo GetTypeInfo(const ComPtr<IMetaDataImport2>& metadata_import,
                     const mdToken& token);

mdAssemblyRef FindAssemblyRef(
    const ComPtr<IMetaDataAssemblyImport>& assembly_import,
    const WSTRING& assembly_name);

// FilterIntegrationsByName removes integrations whose names are specified in
// disabled_integration_names
std::vector<Integration> FilterIntegrationsByName(
    const std::vector<Integration>& integrations,
    const std::vector<WSTRING>& disabled_integration_names);

// FlattenIntegrations flattens integrations to per method structures
std::vector<IntegrationMethod> FlattenIntegrations(
    const std::vector<Integration>& integrations, bool is_calltarget_enabled);

// FilterIntegrationsByCaller removes any integrations which have a caller and
// its not set to the module
std::vector<IntegrationMethod> FilterIntegrationsByCaller(
    const std::vector<IntegrationMethod>& integration_methods,
    const AssemblyInfo assembly);

// FilterIntegrationsByTarget removes any integrations which have a target not
// referenced by the module's assembly import
std::vector<IntegrationMethod> FilterIntegrationsByTarget(
    const std::vector<IntegrationMethod>& integration_methods,
    const ComPtr<IMetaDataAssemblyImport>& assembly_import);

// FilterIntegrationsByTargetAssemblyName removes any integrations which target any
// of the specified assemblies
std::vector<IntegrationMethod> FilterIntegrationsByTargetAssemblyName(
    const std::vector<IntegrationMethod>& integration_methods,
    const std::vector<WSTRING>& excluded_assembly_names);

mdMethodSpec DefineMethodSpec(const ComPtr<IMetaDataEmit2>& metadata_emit,
                              const mdToken& token,
                              const MethodSignature& signature);

bool DisableOptimizations();
bool EnableInlining();
bool IsCallTargetEnabled();

bool TryParseSignatureTypes(const ComPtr<IMetaDataImport2>& metadata_import,
                         const FunctionInfo& function_info,
                         std::vector<WSTRING>& signature_result);

HRESULT CreateAssemblyRefToMscorlib(
    const ComPtr<IMetaDataAssemblyEmit>& assembly_emit,
    mdAssemblyRef* mscorlib_ref);

bool ReturnTypeIsValueTypeOrGeneric(const ComPtr<IMetaDataImport2>& metadata_import,
                      const ComPtr<IMetaDataEmit2>& metadata_emit,
                      const ComPtr<IMetaDataAssemblyEmit>& assembly_emit,
                      const mdToken targetFunctionToken,
                      const MethodSignature targetFunctionSignature,
                      mdToken* ret_type_token);
}  // namespace trace

#endif  // DD_CLR_PROFILER_CLR_HELPERS_H_