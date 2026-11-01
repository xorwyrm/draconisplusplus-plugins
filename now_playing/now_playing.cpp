/**
 * @file now_playing.cpp
 * @brief Now Playing plugin - Cross-platform implementation
 * @author Draconis++ Team
 * @version 1.0.0
 *
 * @details This plugin provides currently playing media information using
 * platform-specific APIs:
 * - Windows: NPSM (Now Playing Session Manager) COM API
 * - Linux/BSD: MPRIS over DBus
 * - macOS: MediaRemote private framework
 */

#include <format>
#include <glaze/glaze.hpp>
#include <utility>

#include <Drac++/Core/Plugin.hpp>

#include <Drac++/Utils/Error.hpp>
#include <Drac++/Utils/Logging.hpp>
#include <Drac++/Utils/Types.hpp>

#include "now_playing_types.hpp"

using namespace draconis::core::plugin;
using namespace draconis::utils::types;
using namespace draconis::utils::error;
// using namespace draconis::utils::logging; // Removed to avoid conflict with MacTypes.h Style
using enum DracErrorCode;

// Glaze metadata for serialization
namespace glz {
  template <>
  struct meta<now_playing::MediaData> {
    using T                     = now_playing::MediaData;
    static constexpr auto value = object(
      "title",
      &T::title,
      "artist",
      &T::artist,
      "album",
      &T::album,
      "playerName",
      &T::playerName
    );
  };
} // namespace glz

#ifdef _WIN32

  #include <propsys.h>
  #include <windows.h>
  #include <wrl/client.h>

namespace now_playing::npsm {
  // {BCBB9860-C012-4AD7-A938-6E337AE6ABA5}
  static const GUID CLSID_NowPlayingSessionManager = {
    .Data1 = 0xBCBB9860,
    .Data2 = 0xC012,
    .Data3 = 0x4AD7,
    .Data4 = { 0xA9, 0x38, 0x6E, 0x33, 0x7A, 0xE6, 0xAB, 0xA5 }
  };

  // INowPlayingSessionManager - {3b6a7908-ce07-4ba9-878c-6e4a15db5e5b} (19041+)
  // These declarations mirror Windows COM interfaces and must retain their ABI.
  // NOLINTBEGIN(cppcoreguidelines-virtual-class-destructor,readability-identifier-naming,misc-use-internal-linkage)
  MIDL_INTERFACE("3b6a7908-ce07-4ba9-878c-6e4a15db5e5b")
  INowPlayingSessionManager : public IUnknown {
   public:
    virtual auto STDMETHODCALLTYPE get_Count(ULONG64 * pCount)->HRESULT               = 0;
    virtual auto STDMETHODCALLTYPE get_CurrentSession(IUnknown * *ppSession)->HRESULT = 0;
  };

  // INowPlayingSession - {431268cf-7477-4285-950b-6f892a944712} (14393+)
  MIDL_INTERFACE("431268cf-7477-4285-950b-6f892a944712")
  INowPlayingSession : public IUnknown {
   public:
    virtual auto STDMETHODCALLTYPE get_SessionType(int* pType) -> HRESULT                            = 0;
    virtual auto STDMETHODCALLTYPE get_SourceAppId(LPWSTR * pszSrcAppId)->HRESULT                    = 0;
    virtual auto STDMETHODCALLTYPE get_SourceDeviceId(LPWSTR * pszSourceDeviceId)->HRESULT           = 0;
    virtual auto STDMETHODCALLTYPE get_RenderDeviceId(LPWSTR * pszRenderId)->HRESULT                 = 0;
    virtual auto STDMETHODCALLTYPE get_HWND(HWND * pHwnd)->HRESULT                                   = 0;
    virtual auto STDMETHODCALLTYPE get_PID(DWORD * pdwPID)->HRESULT                                  = 0;
    virtual auto STDMETHODCALLTYPE get_Info(IUnknown * *ppInfo)->HRESULT                             = 0;
    virtual auto STDMETHODCALLTYPE get_Connection(IUnknown * *ppUnknown)->HRESULT                    = 0;
    virtual auto STDMETHODCALLTYPE ActivateMediaPlaybackDataSource(IUnknown * *ppMediaCtrl)->HRESULT = 0;
  };

  // IMediaPlaybackDataSource - {0F4521BE-A0B8-4116-B3B1-BFECEBAEEBE6} (10586-19041)
  MIDL_INTERFACE("0F4521BE-A0B8-4116-B3B1-BFECEBAEEBE6")
  IMediaPlaybackDataSource : public IUnknown {
   public:
    virtual auto STDMETHODCALLTYPE GetMediaPlaybackInfo(void* pPlaybackInfo) -> HRESULT       = 0;
    virtual auto STDMETHODCALLTYPE SendMediaPlaybackCommand(int command) -> HRESULT           = 0;
    virtual auto STDMETHODCALLTYPE GetMediaObjectInfo(IPropertyStore * *ppPropStore)->HRESULT = 0;
  };

  // IMediaPlaybackDataSource2 - {c4f66b80-df04-4f79-afc2-bee3fc7c46e3} (20279+ / Windows 11)
  MIDL_INTERFACE("c4f66b80-df04-4f79-afc2-bee3fc7c46e3")
  IMediaPlaybackDataSource2 : public IUnknown {
   public:
    virtual auto STDMETHODCALLTYPE GetMediaPlaybackInfo(void* pPlaybackInfo) -> HRESULT       = 0;
    virtual auto STDMETHODCALLTYPE SendMediaPlaybackCommand(int command) -> HRESULT           = 0;
    virtual auto STDMETHODCALLTYPE GetMediaObjectInfo(IPropertyStore * *ppPropStore)->HRESULT = 0;
  };
  // NOLINTEND(cppcoreguidelines-virtual-class-destructor,readability-identifier-naming,misc-use-internal-linkage)
} // namespace now_playing::npsm

  #ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(now_playing::npsm::INowPlayingSessionManager, 0x3b6a7908, 0xce07, 0x4ba9, 0x87, 0x8c, 0x6e, 0x4a, 0x15, 0xdb, 0x5e, 0x5b)
__CRT_UUID_DECL(now_playing::npsm::INowPlayingSession, 0x431268cf, 0x7477, 0x4285, 0x95, 0x0b, 0x6f, 0x89, 0x2a, 0x94, 0x47, 0x12)
__CRT_UUID_DECL(now_playing::npsm::IMediaPlaybackDataSource, 0x0f4521be, 0xa0b8, 0x4116, 0xb3, 0xb1, 0xbf, 0xec, 0xeb, 0xae, 0xeb, 0xe6)
__CRT_UUID_DECL(now_playing::npsm::IMediaPlaybackDataSource2, 0xc4f66b80, 0xdf04, 0x4f79, 0xaf, 0xc2, 0xbe, 0xe3, 0xfc, 0x7c, 0x46, 0xe3)
  #endif

namespace now_playing::npsm {

  // Property keys for media metadata
  // PKEY_Title = {F29F85E0-4FF9-1068-AB91-08002B27B3D9}, 2
  static const PROPERTYKEY PKEY_Title = {
    .fmtid = { .Data1 = 0xF29F85E0, .Data2 = 0x4FF9, .Data3 = 0x1068, .Data4 = { 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9 } },
    .pid   = 2
  };

  // PKEY_Music_Artist = {56A3372E-CE9C-11D2-9F0E-006097C686F6}, 2
  static const PROPERTYKEY PKEY_Music_Artist = {
    .fmtid = { .Data1 = 0x56A3372E, .Data2 = 0xCE9C, .Data3 = 0x11D2, .Data4 = { 0x9F, 0x0E, 0x00, 0x60, 0x97, 0xC6, 0x86, 0xF6 } },
    .pid   = 2
  };

  // PKEY_Music_AlbumTitle = {56A3372E-CE9C-11D2-9F0E-006097C686F6}, 4
  static const PROPERTYKEY PKEY_Music_AlbumTitle = {
    .fmtid = { .Data1 = 0x56A3372E, .Data2 = 0xCE9C, .Data3 = 0x11D2, .Data4 = { 0x9F, 0x0E, 0x00, 0x60, 0x97, 0xC6, 0x86, 0xF6 } },
    .pid   = 4
  };

  namespace {
    /**
     * @brief Converts a wide string (UTF-16) to a UTF-8 encoded string.
     */
    auto ConvertWStringToUTF8(const WString& wstr) -> Result<String> {
      if (wstr.empty())
        return String {};

      const i32 sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<i32>(wstr.length()), nullptr, 0, nullptr, nullptr);

      if (sizeNeeded == 0)
        ERR_FMT(InternalError, "Failed to get buffer size for UTF-8 conversion. Error code: {}", GetLastError());

      String result(static_cast<usize>(sizeNeeded), '\0');

      const i32 bytesConverted =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<i32>(wstr.length()), result.data(), sizeNeeded, nullptr, nullptr);

      if (bytesConverted == 0)
        ERR_FMT(InternalError, "Failed to convert wide string to UTF-8. Error code: {}", GetLastError());

      return result;
    }

    /**
     * @brief Fetch now playing information via Windows NPSM API
     */
    auto FetchNowPlaying() -> Result<MediaData> {
      // Initialize COM if not already initialized
      HRESULT const hrCoInit      = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      const bool    needsCoUninit = SUCCEEDED(hrCoInit);

      Microsoft::WRL::ComPtr<INowPlayingSessionManager> sessionManager;

  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wlanguage-extension-token"
      HRESULT result = CoCreateInstance(
        CLSID_NowPlayingSessionManager,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&sessionManager)
      );
  #pragma clang diagnostic pop

      if (FAILED(result)) {
        if (needsCoUninit)
          CoUninitialize();
        ERR_FMT(ApiUnavailable, "Failed to create NowPlayingSessionManager (HRESULT: 0x{:08X})", static_cast<u32>(result));
      }

      // Check session count for debugging
      ULONG64 sessionCount = 0;
      if (SUCCEEDED(sessionManager->get_Count(&sessionCount)))
        debug_log("Now Playing: Session count = {}", sessionCount);

      Microsoft::WRL::ComPtr<IUnknown> sessionUnknown;
      result = sessionManager->get_CurrentSession(&sessionUnknown);
      if (FAILED(result) || !sessionUnknown) {
        if (needsCoUninit)
          CoUninitialize();
        ERR_FMT(NotFound, "No media session found (HRESULT: 0x{:08X}, sessionCount={})", static_cast<u32>(result), sessionCount);
      }

      Microsoft::WRL::ComPtr<INowPlayingSession> session;
      result = sessionUnknown.As(&session);
      if (FAILED(result)) {
        if (needsCoUninit)
          CoUninitialize();
        ERR_FMT(ApiUnavailable, "Failed to get INowPlayingSession interface (HRESULT: 0x{:08X})", static_cast<u32>(result));
      }

      Microsoft::WRL::ComPtr<IUnknown> dataSourceUnknown;
      result = session->ActivateMediaPlaybackDataSource(&dataSourceUnknown);
      if (FAILED(result) || !dataSourceUnknown) {
        if (needsCoUninit)
          CoUninitialize();
        ERR_FMT(ApiUnavailable, "Failed to activate MediaPlaybackDataSource (HRESULT: 0x{:08X})", static_cast<u32>(result));
      }

      // Try Windows 11 interface first (IMediaPlaybackDataSource2), then fall back to older interface
      Microsoft::WRL::ComPtr<IPropertyStore>            propStore;
      Microsoft::WRL::ComPtr<IMediaPlaybackDataSource2> dataSource2;
      result = dataSourceUnknown.As(&dataSource2);
      if (SUCCEEDED(result)) {
        result = dataSource2->GetMediaObjectInfo(&propStore);
      } else {
        // Fall back to older interface for Windows 10
        Microsoft::WRL::ComPtr<IMediaPlaybackDataSource> dataSource;
        result = dataSourceUnknown.As(&dataSource);
        if (FAILED(result)) {
          if (needsCoUninit)
            CoUninitialize();
          ERR_FMT(ApiUnavailable, "Failed to get IMediaPlaybackDataSource interface (HRESULT: 0x{:08X})", static_cast<u32>(result));
        }
        result = dataSource->GetMediaObjectInfo(&propStore);
      }

      if (FAILED(result) || !propStore) {
        if (needsCoUninit)
          CoUninitialize();
        ERR(ApiUnavailable, "Failed to get media object info");
      }

      MediaData data;

      PROPVARIANT pVar;
      PropVariantInit(&pVar);

      // Get title
      // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
      if (SUCCEEDED(propStore->GetValue(PKEY_Title, &pVar)) && pVar.vt == VT_LPWSTR && pVar.pwszVal)
        if (Result<String> titleStr = ConvertWStringToUTF8(pVar.pwszVal); titleStr && !titleStr->empty())
          data.title = std::move(*titleStr);

      PropVariantClear(&pVar);

      // Get artist
      if (SUCCEEDED(propStore->GetValue(PKEY_Music_Artist, &pVar)) && pVar.vt == VT_LPWSTR && pVar.pwszVal)
        if (Result<String> artistStr = ConvertWStringToUTF8(pVar.pwszVal); artistStr && !artistStr->empty())
          data.artist = std::move(*artistStr);

      PropVariantClear(&pVar);

      // Get album
      if (SUCCEEDED(propStore->GetValue(PKEY_Music_AlbumTitle, &pVar)) && pVar.vt == VT_LPWSTR && pVar.pwszVal)
        if (Result<String> albumStr = ConvertWStringToUTF8(pVar.pwszVal); albumStr && !albumStr->empty())
          data.album = std::move(*albumStr);
      // NOLINTEND(cppcoreguidelines-pro-type-union-access)

      PropVariantClear(&pVar);

      if (needsCoUninit)
        CoUninitialize();
      return data;
    }
  } // anonymous namespace
} // namespace now_playing::npsm

#endif // _WIN32

#ifdef __APPLE__

// macOS implementation is in now_playing_macos.mm (Objective-C++)
// Forward declaration of the function defined there
namespace now_playing::macos {
  auto fetchNowPlaying() -> Result<MediaData>;
} // namespace now_playing::macos

#endif // __APPLE__

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)

  #include <algorithm>
  #include <cerrno>
  #include <charconv>
  #include <chrono>
  #include <cstddef>
  #include <cstring>
  #include <poll.h>
  #include <ranges>
  #include <sys/socket.h>
  #include <sys/uio.h>
  #include <sys/un.h>
  #include <unistd.h>

  #include <Drac++/Utils/Env.hpp>

/**
 * @brief Minimal D-Bus client covering the few calls MPRIS needs.
 *
 * @details Speaks the D-Bus wire protocol directly over the session bus socket
 * instead of linking libdbus, whose static build pulls in a large dependency
 * tree. Only the pieces needed here are implemented: EXTERNAL auth, method
 * calls with string arguments, and reading replies (skipping any value types
 * that aren't needed).
 */
namespace now_playing::dbus {
  using Clock    = std::chrono::steady_clock;
  using Deadline = Clock::time_point;

  enum class MessageType : u8 {
    MethodCall   = 1,
    MethodReturn = 2,
    Error        = 3,
    Signal       = 4,
  };

  enum class HeaderField : u8 {
    Path        = 1,
    Interface   = 2,
    Member      = 3,
    ErrorName   = 4,
    ReplySerial = 5,
    Destination = 6,
    Signature   = 8,
  };

  constexpr usize FIXED_HEADER_SIZE = 16;
  constexpr usize MAX_MESSAGE_SIZE  = 1UZ << 27; // Protocol limit (128 MiB)
  constexpr usize MAX_AUTH_LINE     = 4096;
  constexpr u32   MAX_NESTING_DEPTH = 64;

  constexpr auto AlignUp(const usize value, const usize alignment) -> usize {
    return (value + alignment - 1) / alignment * alignment;
  }

  /**
   * @brief Alignment of a complete type, keyed by its first signature code.
   */
  constexpr auto AlignmentOf(const char code) -> usize {
    if (StringView("nq").contains(code))
      return 2;
    if (StringView("biuhsoa").contains(code))
      return 4;
    if (StringView("xtd({").contains(code))
      return 8;
    return 1; // y, g, v
  }

  /**
   * @brief Returns the index just past the single complete type starting at `pos`.
   */
  auto CompleteTypeEnd(const StringView sig, usize pos) -> Option<usize> {
    if (pos >= sig.size())
      return None;

    switch (sig[pos]) {
      case 'a': return CompleteTypeEnd(sig, pos + 1);
      case '(':
      case '{': {
        const char close = sig[pos] == '(' ? ')' : '}';

        for (++pos; pos < sig.size() && sig[pos] != close;) {
          const Option<usize> next = CompleteTypeEnd(sig, pos);
          if (!next)
            return None;
          pos = *next;
        }

        if (pos >= sig.size())
          return None;

        return pos + 1;
      }
      default:
        if (StringView("ybnqiuxtdhsogv").contains(sig[pos]))
          return pos + 1;
        return None;
    }
  }

  auto IsSingleCompleteType(const StringView sig) -> bool {
    return CompleteTypeEnd(sig, 0) == sig.size();
  }

  /**
   * @brief Serializes values in little-endian D-Bus wire format.
   */
  class MessageWriter {
    Vec<u8> m_buf;

   public:
    auto align(const usize alignment) -> void {
      m_buf.resize(AlignUp(m_buf.size(), alignment), 0);
    }

    auto byte(const u8 value) -> void {
      m_buf.push_back(value);
    }

    auto uint32(const u32 value) -> void {
      align(4);
      for (u32 shift = 0; shift < 32; shift += 8)
        m_buf.push_back(static_cast<u8>(value >> shift));
    }

    auto string(const StringView value) -> void {
      uint32(static_cast<u32>(value.size()));
      m_buf.insert(m_buf.end(), value.begin(), value.end());
      m_buf.push_back(0);
    }

    auto signature(const StringView value) -> void {
      byte(static_cast<u8>(value.size()));
      m_buf.insert(m_buf.end(), value.begin(), value.end());
      m_buf.push_back(0);
    }

    auto bytes(const Span<const u8> data) -> void {
      m_buf.insert(m_buf.end(), data.begin(), data.end());
    }

    auto patchUint32(const usize offset, const u32 value) -> void {
      for (usize i = 0; i < 4; ++i)
        m_buf[offset + i] = static_cast<u8>(value >> (i * 8));
    }

    [[nodiscard]] auto size() const -> usize {
      return m_buf.size();
    }

    [[nodiscard]] auto take() -> Vec<u8> {
      return std::move(m_buf);
    }
  };

  /**
   * @brief Bounds-checked reader over a received message.
   *
   * @details Offsets are relative to the start of the message, which is what
   * D-Bus alignment is defined against. Messages may be in either byte order.
   */
  class MessageReader {
    Span<const u8> m_data;
    usize          m_pos;
    bool           m_bigEndian;

    auto advance(const usize count) -> bool {
      if (count > m_data.size() - m_pos)
        return false;
      m_pos += count;
      return true;
    }

    auto skipValue(const StringView type, const u32 depth) -> bool {
      if (type.empty() || depth > MAX_NESTING_DEPTH)
        return false;

      switch (type[0]) {
        case 'y': return advance(1);
        case 'n':
        case 'q': return align(2) && advance(2);
        case 'b':
        case 'i':
        case 'u':
        case 'h': return align(4) && advance(4);
        case 'x':
        case 't':
        case 'd': return align(8) && advance(8);
        case 's':
        case 'o': return string().has_value();
        case 'g': return signature().has_value();
        case 'v': {
          const Option<StringView> inner = signature();
          return inner && IsSingleCompleteType(*inner) && skipValue(*inner, depth + 1);
        }
        case 'a': {
          const Option<usize> end = beginArray(type[1]);
          if (!end)
            return false;
          m_pos = *end;
          return true;
        }
        case '(':
        case '{': {
          if (!align(8))
            return false;

          const char close = type[0] == '(' ? ')' : '}';

          for (usize pos = 1; type[pos] != close;) {
            const usize next = *CompleteTypeEnd(type, pos);
            if (!skipValue(type.substr(pos, next - pos), depth + 1))
              return false;
            pos = next;
          }

          return true;
        }
        default: return false;
      }
    }

   public:
    MessageReader(const Span<const u8> data, const bool bigEndian, const usize start)
      : m_data(data), m_pos(start), m_bigEndian(bigEndian) {}

    [[nodiscard]] auto position() const -> usize {
      return m_pos;
    }

    auto seek(const usize pos) -> void {
      m_pos = pos;
    }

    auto align(const usize alignment) -> bool {
      return advance(AlignUp(m_pos, alignment) - m_pos);
    }

    auto byte() -> Option<u8> {
      if (!advance(1))
        return None;
      return m_data[m_pos - 1];
    }

    auto uint32() -> Option<u32> {
      if (!align(4) || !advance(4))
        return None;

      u32 value = 0;
      for (usize i = 0; i < 4; ++i) {
        const u32 octet = m_data[m_pos - 4 + i];
        value |= m_bigEndian ? octet << ((3 - i) * 8) : octet << (i * 8);
      }

      return value;
    }

    // Reads a string or object path.
    auto string() -> Option<StringView> {
      const Option<u32> length = uint32();
      if (!length || !advance(*length) || !advance(1) || m_data[m_pos - 1] != 0)
        return None;
      return StringView(reinterpret_cast<const char*>(&m_data[m_pos - *length - 1]), *length);
    }

    auto signature() -> Option<StringView> {
      const Option<u8> length = byte();
      if (!length || !advance(*length) || !advance(1) || m_data[m_pos - 1] != 0)
        return None;
      return StringView(reinterpret_cast<const char*>(&m_data[m_pos - *length - 1]), *length);
    }

    /**
     * @brief Reads an array length and aligns to its first element.
     * @return The offset just past the array's last element.
     */
    auto beginArray(const char elementCode) -> Option<usize> {
      const Option<u32> length = uint32();
      if (!length || !align(AlignmentOf(elementCode)) || *length > m_data.size() - m_pos)
        return None;
      return m_pos + *length;
    }

    // Skips a value whose type comes from the message, validating it first.
    auto skip(const StringView type) -> bool {
      return IsSingleCompleteType(type) && skipValue(type, 0);
    }
  };

  struct Message {
    Vec<u8>     bytes;
    MessageType type        = MessageType::Signal;
    bool        bigEndian   = false;
    u32         replySerial = 0;
    usize       bodyStart   = 0;
    String      signature;
    String      errorName;

    [[nodiscard]] auto body() const -> MessageReader {
      return { bytes, bigEndian, bodyStart };
    }
  };

  /**
   * @brief Parses the header of a complete message received from the bus.
   */
  auto ParseMessage(Vec<u8> bytes) -> Result<Message> {
    Message message;
    message.bigEndian = bytes[0] == 'B';
    message.type      = static_cast<MessageType>(bytes[1]);

    MessageReader reader(bytes, message.bigEndian, 12);
    const usize   fieldsEnd = FIXED_HEADER_SIZE + *reader.uint32();

    while (reader.position() < fieldsEnd) {
      if (!reader.align(8))
        ERR(ParseError, "Malformed D-Bus header field");

      const Option<u8>         code = reader.byte();
      const Option<StringView> sig  = reader.signature();

      if (!code || !sig)
        ERR(ParseError, "Malformed D-Bus header field");

      if (*code == std::to_underlying(HeaderField::ReplySerial) && *sig == "u") {
        const Option<u32> serial = reader.uint32();
        if (!serial)
          ERR(ParseError, "Malformed D-Bus reply serial");
        message.replySerial = *serial;
      } else if (*code == std::to_underlying(HeaderField::Signature) && *sig == "g") {
        const Option<StringView> bodySig = reader.signature();
        if (!bodySig)
          ERR(ParseError, "Malformed D-Bus body signature");
        message.signature = *bodySig;
      } else if (*code == std::to_underlying(HeaderField::ErrorName) && *sig == "s") {
        const Option<StringView> errorName = reader.string();
        if (!errorName)
          ERR(ParseError, "Malformed D-Bus error name");
        message.errorName = *errorName;
      } else if (!reader.skip(*sig)) {
        ERR(ParseError, "Malformed D-Bus header field");
      }
    }

    if (reader.position() != fieldsEnd)
      ERR(ParseError, "D-Bus header fields overran their length");

    message.bodyStart = AlignUp(fieldsEnd, 8);
    message.bytes     = std::move(bytes);
    return message;
  }

  struct MethodCall {
    StringView      destination;
    StringView      path;
    StringView      interface;
    StringView      member;
    Vec<StringView> args; // All arguments are strings
  };

  auto BuildMethodCall(const u32 serial, const MethodCall& call) -> Vec<u8> {
    MessageWriter body;
    for (const StringView arg : call.args)
      body.string(arg);

    MessageWriter header;
    header.byte('l');
    header.byte(std::to_underlying(MessageType::MethodCall));
    header.byte(0); // Flags
    header.byte(1); // Protocol version
    header.uint32(static_cast<u32>(body.size()));
    header.uint32(serial);

    const usize fieldsLengthOffset = header.size();
    header.uint32(0); // Patched below

    const auto addField = [&header](const HeaderField field, const StringView sig, const StringView value) {
      header.align(8);
      header.byte(std::to_underlying(field));
      header.signature(sig);
      if (sig == "g")
        header.signature(value);
      else
        header.string(value);
    };

    addField(HeaderField::Path, "o", call.path);
    addField(HeaderField::Interface, "s", call.interface);
    addField(HeaderField::Member, "s", call.member);
    addField(HeaderField::Destination, "s", call.destination);
    if (!call.args.empty())
      addField(HeaderField::Signature, "g", String(call.args.size(), 's'));

    header.patchUint32(fieldsLengthOffset, static_cast<u32>(header.size() - FIXED_HEADER_SIZE));
    header.align(8);
    header.bytes(body.take());

    return header.take();
  }

  struct SocketAddress {
    sockaddr_un address {};
    socklen_t   length = 0;
  };

  auto MakeSocketAddress(const StringView path, const bool isAbstract) -> Option<SocketAddress> {
    SocketAddress result;
    result.address.sun_family = AF_UNIX;

    // Abstract names start with a NUL byte; filesystem paths end with one.
    if (path.empty() || path.size() + 1 > sizeof(result.address.sun_path))
      return None;

    char* dest = &result.address.sun_path[isAbstract ? 1 : 0];
    path.copy(dest, path.size());
    result.length = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + path.size() + 1);
    return result;
  }

  auto UnescapeAddressValue(const StringView value) -> Option<String> {
    String result;
    for (usize i = 0; i < value.size(); ++i) {
      if (value[i] != '%') {
        result += value[i];
        continue;
      }

      u8 decoded = 0;
      if (i + 2 >= value.size() || std::from_chars(&value[i + 1], &value[i + 3], decoded, 16).ptr != &value[i + 3])
        return None;

      result += static_cast<char>(decoded);
      i += 2;
    }
    return result;
  }

  /**
   * @brief Parses one `unix:` entry of a D-Bus address (e.g. `unix:path=/run/user/1000/bus`).
   */
  auto ParseUnixAddress(StringView entry) -> Option<SocketAddress> {
    constexpr StringView prefix = "unix:";
    if (!entry.starts_with(prefix))
      return None;

    entry.remove_prefix(prefix.size());

    while (!entry.empty()) {
      const usize      comma = entry.find(',');
      const StringView param = entry.substr(0, comma);
      entry                  = comma == StringView::npos ? StringView {} : entry.substr(comma + 1);

      const usize equals = param.find('=');
      if (equals == StringView::npos)
        continue;

      const StringView key   = param.substr(0, equals);
      Option<String>   value = UnescapeAddressValue(param.substr(equals + 1));
      if (!value)
        return None;

      if (key == "path")
        return MakeSocketAddress(*value, false);
  #ifdef __linux__
      if (key == "abstract")
        return MakeSocketAddress(*value, true);
  #endif
    }

    return None;
  }

  auto SessionBusAddresses() -> Vec<SocketAddress> {
    using draconis::utils::env::GetEnv;

    Vec<SocketAddress> result;

    if (Result<String> address = GetEnv("DBUS_SESSION_BUS_ADDRESS"); address && !address->empty()) {
      for (StringView rest = *address; !rest.empty();) {
        const usize semicolon = rest.find(';');
        if (Option<SocketAddress> parsed = ParseUnixAddress(rest.substr(0, semicolon)))
          result.push_back(*parsed);
        rest = semicolon == StringView::npos ? StringView {} : rest.substr(semicolon + 1);
      }
      return result;
    }

    if (Result<String> runtimeDir = GetEnv("XDG_RUNTIME_DIR"); runtimeDir && !runtimeDir->empty())
      if (Option<SocketAddress> parsed = MakeSocketAddress(*runtimeDir + "/bus", false))
        result.push_back(*parsed);

    return result;
  }

  /**
   * @brief Waits until the socket is ready for `events` or the deadline passes.
   */
  auto WaitFor(const i32 fd, const i16 events, const Deadline deadline) -> Result<Unit> {
    while (true) {
      const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - Clock::now());
      if (remaining.count() <= 0)
        ERR(Timeout, "Timed out waiting for D-Bus");

      pollfd pfd { .fd = fd, .events = events, .revents = 0 };

      const i32 ready = poll(&pfd, 1, static_cast<i32>(remaining.count()));
      if (ready > 0)
        return {};
      if (ready < 0 && errno != EINTR)
        ERR_FMT(IoError, "poll() on D-Bus socket failed: {}", std::strerror(errno));
    }
  }

  /**
   * @brief A blocking-style session bus connection with per-call deadlines.
   */
  class Connection {
    i32     m_fd     = -1;
    u32     m_serial = 0;
    Vec<u8> m_input; // Bytes received but not yet consumed

    explicit Connection(const i32 fd) : m_fd(fd) {}

    auto sendAll(Span<const u8> data, const Deadline deadline) -> Result<Unit> {
      while (!data.empty()) {
        const isize sent = send(m_fd, data.data(), data.size(), MSG_NOSIGNAL);

        if (sent > 0) {
          data = data.subspan(static_cast<usize>(sent));
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
          TRY_VOID(WaitFor(m_fd, POLLOUT, deadline));
        } else if (errno != EINTR) {
          ERR_FMT(IoError, "Failed to write to D-Bus: {}", std::strerror(errno));
        }
      }
      return {};
    }

    auto sendAll(const StringView text, const Deadline deadline) -> Result<Unit> {
      return sendAll(Span(reinterpret_cast<const u8*>(text.data()), text.size()), deadline);
    }

    // Sends the NUL byte that starts authentication. FreeBSD and DragonFly need
    // the sender's credentials attached to it for the bus to identify us.
    auto sendCredentialsByte(const Deadline deadline) -> Result<Unit> {
  #if defined(__FreeBSD__) || defined(__DragonFly__)
      u8    nul = 0;
      iovec iov { .iov_base = &nul, .iov_len = 1 };

      // The kernel fills in the credentials; we only provide the space.
      alignas(cmsghdr) Array<u8, CMSG_SPACE(sizeof(cmsgcred))> control {};

      msghdr message {};
      message.msg_iov        = &iov;
      message.msg_iovlen     = 1;
      message.msg_control    = control.data();
      message.msg_controllen = static_cast<socklen_t>(control.size());

      cmsghdr* cmsg    = CMSG_FIRSTHDR(&message);
      cmsg->cmsg_len   = CMSG_LEN(sizeof(cmsgcred));
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type  = SCM_CREDS;

      while (sendmsg(m_fd, &message, MSG_NOSIGNAL) != 1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          TRY_VOID(WaitFor(m_fd, POLLOUT, deadline));
        else if (errno != EINTR)
          ERR_FMT(IoError, "Failed to send D-Bus credentials: {}", std::strerror(errno));
      }
      return {};
  #else
      return sendAll(StringView("\0", 1), deadline);
  #endif
    }

    auto fill(const Deadline deadline) -> Result<Unit> {
      Array<u8, 4096> chunk {};

      while (true) {
        const isize received = recv(m_fd, chunk.data(), chunk.size(), 0);

        if (received > 0) {
          m_input.insert(m_input.end(), chunk.begin(), chunk.begin() + received);
          return {};
        }

        if (received == 0)
          ERR(IoError, "D-Bus connection closed");

        if (errno == EAGAIN || errno == EWOULDBLOCK)
          TRY_VOID(WaitFor(m_fd, POLLIN, deadline));
        else if (errno != EINTR)
          ERR_FMT(IoError, "Failed to read from D-Bus: {}", std::strerror(errno));
      }
    }

    auto readLine(const Deadline deadline) -> Result<String> {
      while (true) {
        const StringView buffered(reinterpret_cast<const char*>(m_input.data()), m_input.size());

        if (const usize end = buffered.find("\r\n"); end != StringView::npos) {
          String line(buffered.substr(0, end));
          m_input.erase(m_input.begin(), m_input.begin() + static_cast<isize>(end + 2));
          return line;
        }

        if (m_input.size() > MAX_AUTH_LINE)
          ERR(ParseError, "D-Bus authentication line too long");

        TRY_VOID(fill(deadline));
      }
    }

    auto readMessage(const Deadline deadline) -> Result<Message> {
      while (m_input.size() < FIXED_HEADER_SIZE)
        TRY_VOID(fill(deadline));

      const u8 endianness = m_input[0];
      if ((endianness != 'l' && endianness != 'B') || m_input[3] != 1)
        ERR(ParseError, "Unsupported D-Bus message encoding");

      MessageReader fixedHeader(m_input, endianness == 'B', 4);
      const usize   bodyLength = *fixedHeader.uint32();
      fixedHeader.seek(12);
      const usize fieldsLength = *fixedHeader.uint32();

      if (bodyLength > MAX_MESSAGE_SIZE || fieldsLength > MAX_MESSAGE_SIZE)
        ERR(ParseError, "D-Bus message too large");

      const usize totalLength = AlignUp(FIXED_HEADER_SIZE + fieldsLength, 8) + bodyLength;
      if (totalLength > MAX_MESSAGE_SIZE)
        ERR(ParseError, "D-Bus message too large");

      while (m_input.size() < totalLength)
        TRY_VOID(fill(deadline));

      Vec<u8> bytes(m_input.begin(), m_input.begin() + static_cast<isize>(totalLength));
      m_input.erase(m_input.begin(), m_input.begin() + static_cast<isize>(totalLength));

      return ParseMessage(std::move(bytes));
    }

    auto authenticate(const Deadline deadline) -> Result<Unit> {
      TRY_VOID(sendCredentialsByte(deadline));

      // EXTERNAL auth identifies us by UID, sent as hex-encoded ASCII digits.
      String hexUid;
      for (const char digit : std::to_string(geteuid()))
        hexUid += std::format("{:02x}", digit);

      TRY_VOID(sendAll(std::format("AUTH EXTERNAL {}\r\n", hexUid), deadline));

      if (const String reply = TRY(readLine(deadline)); !reply.starts_with("OK "))
        ERR_FMT(PermissionDenied, "D-Bus authentication failed: {}", reply);

      return sendAll("BEGIN\r\n", deadline);
    }

   public:
    Connection(const Connection&)                    = delete;
    auto operator=(const Connection&) -> Connection& = delete;

    Connection(Connection&& other) noexcept
      : m_fd(std::exchange(other.m_fd, -1)), m_serial(other.m_serial), m_input(std::move(other.m_input)) {}

    auto operator=(Connection&& other) noexcept -> Connection& {
      if (this != &other) {
        if (m_fd >= 0)
          close(m_fd);
        m_fd     = std::exchange(other.m_fd, -1);
        m_serial = other.m_serial;
        m_input  = std::move(other.m_input);
      }
      return *this;
    }

    ~Connection() {
      if (m_fd >= 0)
        close(m_fd);
    }

    /**
     * @brief Connects and authenticates to the session bus.
     */
    static auto openSession(const std::chrono::milliseconds timeout) -> Result<Connection> {
      const Vec<SocketAddress> addresses = SessionBusAddresses();
      if (addresses.empty())
        ERR(ApiUnavailable, "No usable D-Bus session bus address (DBUS_SESSION_BUS_ADDRESS / XDG_RUNTIME_DIR)");

      const Deadline deadline  = Clock::now() + timeout;
      i32            lastErrno = 0;

      for (const SocketAddress& address : addresses) {
        const i32 fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0)
          ERR_FMT(IoError, "Failed to create socket: {}", std::strerror(errno));

        Connection connection(fd);

        if (connect(fd, reinterpret_cast<const sockaddr*>(&address.address), address.length) != 0) {
          lastErrno = errno;
          continue;
        }

        TRY_VOID(connection.authenticate(deadline));

        // The bus requires Hello before any other call.
        TRY_VOID(connection.callUntil(
          {
            .destination = "org.freedesktop.DBus",
            .path        = "/org/freedesktop/DBus",
            .interface   = "org.freedesktop.DBus",
            .member      = "Hello",
            .args        = {},
          },
          deadline
        ));

        return connection;
      }

      ERR_FMT(ApiUnavailable, "Failed to connect to the D-Bus session bus: {}", std::strerror(lastErrno));
    }

    auto callUntil(const MethodCall& call, const Deadline deadline) -> Result<Message> {
      const u32 serial = ++m_serial;
      TRY_VOID(sendAll(BuildMethodCall(serial, call), deadline));

      while (true) {
        Message reply = TRY(readMessage(deadline));

        // Signals (e.g. NameAcquired) can arrive in between; skip them.
        if (reply.replySerial != serial || (reply.type != MessageType::MethodReturn && reply.type != MessageType::Error))
          continue;

        if (reply.type == MessageType::Error) {
          Option<StringView> detail;
          if (reply.signature.starts_with('s'))
            detail = reply.body().string();
          ERR_FMT(ApiUnavailable, "D-Bus {} failed: {} {}", call.member, reply.errorName, detail.value_or(""));
        }

        return reply;
      }
    }

    auto call(const MethodCall& call, const std::chrono::milliseconds timeout) -> Result<Message> {
      return callUntil(call, Clock::now() + timeout);
    }
  };

  auto NonEmpty(const Option<StringView> value) -> Option<String> {
    if (!value || value->empty())
      return None;
    return String(*value);
  }

  /**
   * @brief Extract player name from MPRIS bus name
   */
  auto extractPlayerName(const String& busName) -> String {
    constexpr StringView prefix = "org.mpris.MediaPlayer2.";
    if (busName.starts_with(prefix))
      return busName.substr(prefix.size());
    return busName;
  }

  constexpr auto CALL_TIMEOUT = std::chrono::milliseconds(100);

  /**
   * @brief Lists the MPRIS players on the bus, sorted by bus name.
   */
  auto ListPlayers(Connection& connection) -> Result<Vec<String>> {
    const Message reply = TRY(connection.call(
      {
        .destination = "org.freedesktop.DBus",
        .path        = "/org/freedesktop/DBus",
        .interface   = "org.freedesktop.DBus",
        .member      = "ListNames",
        .args        = {},
      },
      CALL_TIMEOUT
    ));

    if (reply.signature != "as")
      ERR_FMT(ParseError, "Unexpected ListNames reply signature '{}'", reply.signature);

    MessageReader       reader = reply.body();
    const Option<usize> end    = reader.beginArray('s');
    if (!end)
      ERR(ParseError, "Malformed ListNames reply");

    Vec<String> players;
    while (reader.position() < *end) {
      const Option<StringView> name = reader.string();
      if (!name)
        ERR(ParseError, "Malformed ListNames reply");
      if (name->starts_with("org.mpris.MediaPlayer2."))
        players.emplace_back(*name);
    }

    std::ranges::sort(players);
    return players;
  }

  /**
   * @brief Reads a player's PlaybackStatus ("Playing", "Paused" or "Stopped").
   */
  auto GetPlaybackStatus(Connection& connection, const String& player) -> Result<String> {
    const MethodCall getStatus {
      .destination = player,
      .path        = "/org/mpris/MediaPlayer2",
      .interface   = "org.freedesktop.DBus.Properties",
      .member      = "Get",
      .args        = { "org.mpris.MediaPlayer2.Player", "PlaybackStatus" },
    };

    const Message reply = TRY(connection.call(getStatus, CALL_TIMEOUT));

    MessageReader reader = reply.body();
    if (reply.signature != "v" || reader.signature() != "s")
      ERR(ParseError, "PlaybackStatus is not a string");

    const Option<StringView> status = reader.string();
    if (!status)
      ERR(ParseError, "Malformed PlaybackStatus reply");

    return String(*status);
  }

  /**
   * @brief Reads a player's track metadata.
   */
  auto FetchPlayer(Connection& connection, const String& player) -> Result<MediaData> {
    const MethodCall getMetadata {
      .destination = player,
      .path        = "/org/mpris/MediaPlayer2",
      .interface   = "org.freedesktop.DBus.Properties",
      .member      = "Get",
      .args        = { "org.mpris.MediaPlayer2.Player", "Metadata" },
    };

    const Message reply = TRY(connection.call(getMetadata, CALL_TIMEOUT));

    if (reply.signature != "v")
      ERR(ParseError, "Properties.Get reply argument is not a variant");

    MessageReader reader = reply.body();
    if (reader.signature() != "a{sv}")
      ERR(ParseError, "Metadata is not a dictionary array");

    const Option<usize> end = reader.beginArray('{');
    if (!end)
      ERR(ParseError, "Malformed metadata dictionary");

    MediaData data;
    data.playerName = extractPlayerName(player);

    while (reader.position() < *end) {
      if (!reader.align(8))
        ERR(ParseError, "Malformed metadata entry");

      const Option<StringView> key       = reader.string();
      const Option<StringView> valueType = reader.signature();
      if (!key || !valueType)
        ERR(ParseError, "Malformed metadata entry");

      bool ok = true;

      if (*key == "xesam:title" && *valueType == "s") {
        const Option<StringView> value = reader.string();
        ok                             = value.has_value();
        data.title                     = NonEmpty(value);
      } else if (*key == "xesam:album" && *valueType == "s") {
        const Option<StringView> value = reader.string();
        ok                             = value.has_value();
        data.album                     = NonEmpty(value);
      } else if (*key == "xesam:artist" && *valueType == "as") {
        const Option<usize> artistsEnd = reader.beginArray('s');
        ok                             = artistsEnd.has_value();
        if (ok && reader.position() < *artistsEnd)
          data.artist = NonEmpty(reader.string());
        if (ok)
          reader.seek(*artistsEnd);
      } else {
        ok = reader.skip(*valueType);
      }

      if (!ok)
        ERR_FMT(ParseError, "Malformed metadata value for '{}'", *key);
    }

    return data;
  }

  /**
   * @brief Fetch now playing information via MPRIS/DBus
   *
   * @details Prefers a playing player, then a paused one, then any other, and
   * returns the first that reports a title. Players that don't answer are
   * skipped, within an overall time budget.
   */
  auto fetchNowPlaying() -> Result<MediaData> {
    Connection connection = TRY(Connection::openSession(std::chrono::milliseconds(1000)));

    const Vec<String> players  = TRY(ListPlayers(connection));
    const Deadline    deadline = Clock::now() + std::chrono::seconds(3);

    Vec<Pair<i32, String>> ranked;
    for (const String& player : players) {
      if (Clock::now() >= deadline)
        break;

      const Result<String> status = GetPlaybackStatus(connection, player);
      if (!status)
        continue;

      ranked.emplace_back(*status == "Playing" ? 0 : *status == "Paused" ? 1
                                                                         : 2,
                          player);
    }

    std::ranges::sort(ranked);

    for (const String& player : ranked | std::views::values) {
      if (Clock::now() >= deadline)
        break;

      if (Result<MediaData> data = FetchPlayer(connection, player); data && data->title)
        return data;
    }

    ERR(NotFound, "No responsive MPRIS player has media metadata");
  }
} // namespace now_playing::dbus

#endif // Linux/BSD

namespace {
  class NowPlayingPlugin : public IInfoProviderPlugin {
   private:
    PluginMetadata                m_metadata;
    now_playing::NowPlayingConfig m_config;
    now_playing::MediaData        m_data;
    Option<String>                m_lastError;
    bool                          m_ready = false;

   public:
    NowPlayingPlugin() {
      m_metadata = {
        .name    = "Now Playing",
        .version = "1.0.0",
        .author  = "Draconis++ Team",
#ifdef _WIN32
        .description = "Provides currently playing media information via Windows NPSM",
#elifdef __APPLE__
        .description = "Provides currently playing media information via macOS MediaRemote",
#else
        .description = "Provides currently playing media information via MPRIS/DBus",
#endif
        .type         = PluginType::InfoProvider,
        .dependencies = { .requiresNetwork = false, .requiresCaching = true },
      };
    }

    [[nodiscard]] auto getMetadata() const -> const PluginMetadata& override {
      return m_metadata;
    }

    [[nodiscard]] auto getProviderId() const -> String override {
      return "now_playing";
    }

    auto setConfig(StringView tomlConfig) -> Result<Unit> override {
      if (tomlConfig.empty())
        return {};

      // Parse TOML config for now_playing
      // Currently only 'enabled' field is supported
      // Example: enabled = true
      const String configStr(tomlConfig);

      // Simple parsing - look for enabled = true/false
      if (configStr.contains("enabled = false") || configStr.contains("enabled=false"))
        m_config.enabled = false;
      else if (configStr.contains("enabled = true") || configStr.contains("enabled=true"))
        m_config.enabled = true;

      debug_log("Now Playing plugin: received runtime config, enabled={}", m_config.enabled);
      return {};
    }

    auto initialize(const PluginContext& /*ctx*/, PluginCache& /*cache*/) -> Result<Unit> override {
      // Config already set via setConfig() or defaults to enabled=true
      m_ready = true;
      return {};
    }

    auto shutdown() -> Unit override {
      m_ready = false;
    }

    [[nodiscard]] auto isReady() const -> bool override {
      return m_ready;
    }

    [[nodiscard]] auto isEnabled() const -> bool override {
      return m_config.enabled;
    }

    auto collectData(PluginCache& /*cache*/) -> Result<Unit> override {
      if (!m_ready)
        ERR(NotSupported, "Now Playing plugin is not ready");

      if (!m_config.enabled) {
        m_lastError = "Now Playing plugin is disabled";
        return {};
      }

      m_lastError = None;

      // Fetch fresh data using platform-specific implementation (no caching - media changes too frequently)
#ifdef _WIN32
      auto result = now_playing::npsm::FetchNowPlaying();
#elifdef __APPLE__
      auto result = now_playing::macos::fetchNowPlaying();
#else
      auto result = now_playing::dbus::fetchNowPlaying();
#endif

      if (!result) {
        m_lastError = result.error().message;
        return std::unexpected(result.error());
      }

      m_data = *result;

      return {};
    }

    [[nodiscard]] auto getFields() const -> PluginFields override {
      PluginFields fields;

      if (m_data.title)
        fields["title"] = *m_data.title;

      if (m_data.artist)
        fields["artist"] = *m_data.artist;

      if (m_data.album)
        fields["album"] = *m_data.album;

      if (m_data.playerName)
        fields["player"] = *m_data.playerName;

      return fields;
    }

    [[nodiscard]] auto getDisplayValue() const -> Result<String> override {
      if (!m_data.title)
        ERR(NotFound, "No media currently playing");

      if (m_data.artist)
        return std::format("{} - {}", *m_data.artist, *m_data.title);

      return *m_data.title;
    }

    [[nodiscard]] auto getDisplayIcon() const -> String override {
      return " 󰝚  "; // Nerd Font music icon
    }

    [[nodiscard]] auto getDisplayLabel() const -> String override {
      return "Playing";
    }

    [[nodiscard]] auto getLastError() const -> Option<String> override {
      return m_lastError;
    }
  };
} // namespace

DRAC_PLUGIN(NowPlayingPlugin)
