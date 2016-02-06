//===--------------------- filesystem/ops.cpp -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "experimental/filesystem"
#include "iterator"
#include "fstream"
#include "type_traits"
#include "random"  /* for unique_path */
#include "cstdlib"
#include "climits"

#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>  /* values for fchmodat */


_LIBCPP_BEGIN_NAMESPACE_EXPERIMENTAL_FILESYSTEM

filesystem_error::~filesystem_error() {}

////////////////////////////////////////////////////////////////////////////////
//                       POSIX HELPERS                                                 
////////////////////////////////////////////////////////////////////////////////
namespace detail { namespace  {

using value_type = path::value_type;
using string_type = path::string_type;

////////////////////////////////////////////////////////////////////////
inline std::error_code capture_errno() {
    _LIBCPP_ASSERT(errno, "Expected errno to be non-zero");
    return std::error_code{errno, std::system_category()};
}

////////////////////////////////////////////////////////////////////////
// set the permissions as described in stat
perms posix_get_perms(const struct ::stat & st) noexcept
{
    return static_cast<perms>(st.st_mode) & perms::mask;
}

////////////////////////////////////////////////////////////////////////
::mode_t posix_convert_perms(perms prms)
{
    return static_cast< ::mode_t>(
        prms & perms::mask
      );
}

////////////////////////////////////////////////////////////////////////
file_status posix_stat(
    path const & p
  , struct ::stat & path_stat
  , std::error_code* ec
  )
{
    std::error_code m_ec;
    
    if (::stat(p.c_str(), &path_stat) == -1) {
        m_ec = detail::capture_errno();
    }
    
    if (ec) *ec = m_ec;
        
    if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
        return file_status(file_type::not_found);
    } 
    else if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::posix_stat", p, m_ec);
    }
    else if (m_ec) {
        return file_status(file_type::none);
    }
    // else
    
    file_status fs_tmp;
    auto const mode = path_stat.st_mode;
    if      (S_ISREG(mode))  fs_tmp.type(file_type::regular);
    else if (S_ISDIR(mode))  fs_tmp.type(file_type::directory);
    else if (S_ISBLK(mode))  fs_tmp.type(file_type::block);
    else if (S_ISCHR(mode))  fs_tmp.type(file_type::character);
    else if (S_ISFIFO(mode)) fs_tmp.type(file_type::fifo);
    else if (S_ISSOCK(mode)) fs_tmp.type(file_type::socket);
    else                     fs_tmp.type(file_type::unknown);
    
    fs_tmp.permissions(detail::posix_get_perms(path_stat));
    
    return fs_tmp;
}
            
////////////////////////////////////////////////////////////////////////
file_status posix_lstat(
    path const & p
  , struct ::stat & path_stat
  , std::error_code* ec
  )
{
    std::error_code m_ec;
    
    if (::lstat(p.c_str(), &path_stat) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
    
    if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
        return file_status(file_type::not_found);
    }
    else if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::posix_lstat", p, m_ec);
    }
    else if (m_ec) {
        return file_status(file_type::none);
    }
    // else
    
    file_status fs_tmp;
    auto const mode = path_stat.st_mode;
    if      (S_ISLNK(mode))  fs_tmp.type(file_type::symlink);
    else if (S_ISREG(mode))  fs_tmp.type(file_type::regular);
    else if (S_ISDIR(mode))  fs_tmp.type(file_type::directory);
    else if (S_ISBLK(mode))  fs_tmp.type(file_type::block);
    else if (S_ISCHR(mode))  fs_tmp.type(file_type::character);
    else if (S_ISFIFO(mode)) fs_tmp.type(file_type::fifo);
    else if (S_ISSOCK(mode)) fs_tmp.type(file_type::socket);
    else                     fs_tmp.type(file_type::unknown);
    
    // TODO symbolic link
    fs_tmp.permissions(detail::posix_get_perms(path_stat));
    return fs_tmp;
}

////////////////////////////////////////////////////////////////////////////////
//                           DETAIL::MISC                                           
////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
bool copy_file_impl(
    const path& from, const path& to
  , std::error_code *ec
  )
{
    if (ec) { ec->clear(); }
        
    std::ifstream in(from.c_str(), std::ios::binary);
    std::ofstream out(to.c_str(),  std::ios::binary);
    
    if (in.good() && out.good()) {
        using InIt = std::istreambuf_iterator<char>;
        using OutIt = std::ostreambuf_iterator<char>;
        InIt bin(in);
        InIt ein;
        OutIt bout(out);
        std::copy(bin, ein, bout);
    }
    
    if (out.fail() || in.fail()) {
        std::error_code m_ec(
            static_cast<int>(std::errc::operation_not_permitted)
          , std::system_category()
          );
   
        if (ec) {
            *ec = m_ec;
            return false;
        } else {
            throw filesystem_error("std::experimental::filesystem::copy_file_impl", from, to, m_ec);
        }
    }
    return true;
}

}} // end namespace detail

////////////////////////////////////////////////////////////////////////////////
//                        DETAIL::OPERATORS DEFINITIONS                                                        
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
path __canonical(path const & orig_p, const path& base, std::error_code *ec)
{
    path p = absolute(orig_p, base);
    
    char buff[PATH_MAX + 1];
    char *ret = ::realpath(p.c_str(), buff);
        
    std::error_code m_ec;
    if (not ret) {
        m_ec = detail::capture_errno();
        _LIBCPP_ASSERT(m_ec, "Expected error code");
    }
    if (ec) *ec = m_ec;
            
    if (m_ec && not ec) { 
        throw filesystem_error("std::experimental::filesystem::canonical", orig_p, m_ec);
    }
    else if (m_ec) {
        return {};
    } else {
        return {ret};
    }
}

////////////////////////////////////////////////////////////////////////////
void __copy(
    const path& from, const path& to
  , copy_options options
  , std::error_code *ec
  )
{
    const bool sym_status = bool(options & 
        (copy_options::create_symlinks | copy_options::skip_symlinks));
    
    std::error_code m_ec;
    struct ::stat f_st;
    const file_status f = sym_status ? detail::posix_lstat(from, f_st, &m_ec)
                                     : detail::posix_stat(from, f_st, &m_ec);
    
    if (m_ec && ec) {
        *ec = m_ec;
        return;
    } else if (m_ec) {
        throw filesystem_error("std::experimental::filesystem::copy", from, m_ec);
    }
    
    
    struct ::stat t_st;
    const file_status t = sym_status ? detail::posix_lstat(to, t_st, &m_ec)
                                     : detail::posix_stat(to, t_st, &m_ec);
    
    if (not status_known(t)) {
        if (ec) {
            *ec = m_ec;
            return;
        } else {
            throw filesystem_error("std::experimental::filesystem::copy", from, to, m_ec);
        }
    }
    
    
    if ( is_other(f) || is_other(t) 
        || (is_directory(f) && is_regular_file(t))
        || equivalent(from, to, m_ec))
    {
        const std::error_code mec(
            static_cast<int>(std::errc::function_not_supported)
          , std::system_category()
          );
        if (ec) {
            *ec = mec;
            return;
        } else {
            throw filesystem_error("std::experimental::filesystem::copy", from, to, mec);
        }
    }
    
    if (ec) { ec->clear(); }
        
    if (is_symlink(f)) {
        if (bool(copy_options::skip_symlinks & options)) {
            // do nothing
        }
        else if (not exists(t)) {
            __copy_symlink(from, to, ec);
        } else {
            const std::error_code mec(
                static_cast<int>(std::errc::file_exists)
              , std::system_category()
              );
            if (ec) {
                *ec = mec;
            } else {
                throw filesystem_error("std::experimental::filesystem::copy", from, to, mec);
            }
        }
    }
    else if (is_regular_file(f)) {
        if (bool(copy_options::directories_only & options)) {
            // do nothing
        }
        else if (bool(copy_options::create_symlinks & options)) {
            __create_symlink(from, to, ec);
        }
        else if (bool(copy_options::create_hard_links & options)) {
            __create_hard_link(from, to, ec);
        }
        else if (is_directory(t)) {
            __copy_file(from, to/from.filename(), options, ec);
        } else {
            __copy_file(from, to, options, ec);
        }
    }
    else if (is_directory(f)) {
        if (not bool(copy_options::recursive & options) &&
            bool(copy_options::__in_recursive_copy & options))
        {
            return;
        }
        
        if (!exists(t)) {
            // create directory to with attributes from 'from'.
            __create_directory(to, from, ec);
            if (ec && *ec) { return; }
        }
        directory_iterator it = ec ? directory_iterator(from, *ec) 
                                   : directory_iterator(from);
        if (ec && *ec) { return; }
        for (; it != directory_iterator(); ++it) {
            __copy(it->path(), to / it->path().filename()
              , options | copy_options::__in_recursive_copy
              , ec
              );
            if (ec && *ec) { return; }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
bool __copy_file(
    const path& from, const path& to
  , copy_options options, std::error_code *ec
  )
{
    if (ec) ec->clear();
        
    std::error_code m_ec;
    const bool good_from = is_regular_file(from, m_ec);
    if (not good_from) {
        if (not m_ec) {
            m_ec = std::error_code(
                static_cast<int>(std::errc::no_such_file_or_directory)
                , std::system_category()
                );
        }
            
        if (ec) {
            *ec = m_ec;
            return false;
        } else {
            throw filesystem_error(
                "std::experimental::filesystem::copy_file source file is not a regular file"
            , from, to, m_ec
            );
        }
    }
    
    const bool to_exists = exists(to, m_ec);
    
    if (to_exists && not bool(options & 
        (copy_options::skip_existing 
        | copy_options::update_existing 
        | copy_options::overwrite_existing))) 
    {
        const std::error_code mec(
            static_cast<int>(std::errc::file_exists)
          , std::system_category()
          );
        if (ec) {
            *ec = mec;
            return false;
        } else {
            throw filesystem_error("std::experimental::filesystem::copy_file", from, to, mec);
        }
    }
    else if (to_exists && bool(copy_options::skip_existing & options)) {
        return false;
    }
    else if (to_exists && bool(copy_options::update_existing & options)) {
        auto from_time = __last_write_time(from, ec);
        if (ec && *ec) { return false; }
        auto to_time = __last_write_time(to, ec);
        if (ec && *ec) { return false; }
        if (from_time <= to_time) {
            return false;
        }
    }
    return detail::copy_file_impl(from, to, ec);
}
    

void __copy_symlink(
    const path& existing_symlink, const path& new_symlink
  , std::error_code *ec)
{
    const path real_path(__read_symlink(existing_symlink, ec));
    if (ec && *ec) { return; }
    // TODO
    // NOTE: proposal says you should detect if you should call
    // create_symlink or create_directory_symlink. I don't think this
    // is needed with POSIX
    __create_symlink(real_path, new_symlink, ec);
}


bool __create_directories(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    auto const st = status(p, m_ec);
    
    if (is_directory(st)) {
        if (ec) ec->clear();
        return false;
    }
    
    const path parent = p.parent_path();
    if (!parent.empty()) {
        const file_status parent_st = status(parent, m_ec);
        if (not status_known(parent_st)) {
            _LIBCPP_ASSERT(m_ec, "Expected error code");
            if (ec) {
                *ec = m_ec;
                return false;
            } else {
                throw filesystem_error("std::experimental::filesystem::create_directories", p, m_ec);
            }
        }
        if (not exists(parent_st)) {
            __create_directories(parent, ec);
            if (ec && *ec) { return false; }
        }
    }
    
    return __create_directory(p, ec);
}


bool __create_directory(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    if (::mkdir(p.c_str(), S_IRWXU|S_IRWXG|S_IRWXO) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
    
    if (not m_ec) return true;
        
    if (m_ec.value() == EEXIST && is_directory(p)) {
        if (ec) ec->clear();
        return false;
    }
    else if (not ec) {
        throw filesystem_error("std::experimental::filesystem::create_directory", p, m_ec);
    } else {
        return false;
    }
}


bool __create_directory(
    path const & p, path const & attributes
  , std::error_code *ec
  )
{
    struct ::stat attr_stat;
    std::error_code mec;
    detail::posix_stat(attributes, attr_stat, &mec);
    if (mec && ec) {
        *ec = mec;
        return false;
    }
    else if (mec) {
        throw filesystem_error("std::experimental::filesystem::create_directory", p, attributes, mec);
    }

    if (ec) ec->clear();
        
    if (::mkdir(p.c_str(), attr_stat.st_mode) == -1) {
        mec = std::error_code(errno, std::system_category());
        if (mec.value() == EEXIST && is_directory(p)) {
            return false;
        }
        else if (ec) {
            *ec = mec;
            return false;
        } else {
            throw filesystem_error("std::experimental::filesystem::create_directory", p, attributes, mec);
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
void __create_directory_symlink(
    path const & from, path const & to
  , std::error_code *ec
  )
{
    std::error_code m_ec;
    if (::symlink(from.c_str(), to.c_str()) != 0) {
        m_ec = detail::capture_errno();
        _LIBCPP_ASSERT(m_ec, "Expected error code");
    }
    if (ec) *ec = m_ec;
            
    if (m_ec && not ec) {
        throw filesystem_error(
            "std::experimental::filesystem::create_directory_symlink"
          , from, to, m_ec
          );
    }
}

////////////////////////////////////////////////////////////////////////////
void __create_hard_link(
    const path& from, const path& to,
    std::error_code *ec
  )
{
    std::error_code m_ec;
    if (::link(from.c_str(), to.c_str()) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::create_hard_link", from, to, m_ec);
    }
}

////////////////////////////////////////////////////////////////////////////
void __create_symlink(
    path const & from, path const & to
  , std::error_code *ec
  )
{
    std::error_code m_ec;
    if (::symlink(from.c_str(), to.c_str()) != 0) {
        m_ec = detail::capture_errno();
        _LIBCPP_ASSERT(m_ec, "Expected error code");
    }
    if (ec) *ec = m_ec;
            
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::create_symlink", from, to, m_ec);
    }
}

////////////////////////////////////////////////////////////////////////////
path __current_path(std::error_code *ec)
{
    auto size = ::pathconf(".", _PC_PATH_MAX);
    _LIBCPP_ASSERT(size >= 0, "pathconf returned a 0 as max size");

    auto buff = std::unique_ptr<char[]>(new char[size + 1]);
        
    char* ret = ::getcwd(buff.get(), static_cast<size_t>(size));
        
    std::error_code m_ec;
    if (not ret) {
            m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::current_path", m_ec);
    }
    else if (m_ec) {
        return {};
    } else {
        return {buff.get()};
    }
}

////////////////////////////////////////////////////////////////////////////
void __current_path(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    if (::chdir(p.c_str()) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
            
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::current_path", p, m_ec);
    }

}

////////////////////////////////////////////////////////////////////////////
bool __equivalent(
    const path& p1, const path& p2, 
    std::error_code *ec)
{
    std::error_code ec1, ec2;
    struct ::stat st1, st2;
    detail::posix_stat(p1.native(), st1, &ec1);
    detail::posix_stat(p2.native(), st2, &ec2);
    
    if (ec1 && ec2) {
        if (ec) ec->clear();
        return false;
    }
    else if (ec1 || ec2) {
        if (ec) {
            *ec = ec1 ? ec1 : ec2;
            return false;
        } else {
            throw filesystem_error(
              "std::experimental::filesystem::equivalent"
            , p1, p2
            , ec1 ? ec1 : ec2
            );
        }
    } else {
        if (ec) ec->clear();
        return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino);
    }
}

////////////////////////////////////////////////////////////////////////////
std::uintmax_t __file_size(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    struct ::stat st;
    file_status fst = detail::posix_stat(p, st, &m_ec);
    if (is_regular_file(fst)) {
        _LIBCPP_ASSERT(not m_ec, "expected no error code");
        if (ec) ec->clear();
        return static_cast<std::uintmax_t>(st.st_size);
    }
    else if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::file_size", p, m_ec);
    }
    else if (m_ec) {
        *ec = m_ec;
        return static_cast<std::uintmax_t>(-1);
    // else !is_regular_file(fst)
    } else {
        std::error_code tmp_ec(EPERM, std::system_category());
        if (ec) {
            *ec = tmp_ec;
            return static_cast<std::uintmax_t>(-1);
        } else {
            throw filesystem_error("std::experimental::filesystem::file_size", p, tmp_ec);
        }
    } 
}

////////////////////////////////////////////////////////////////////////////
std::uintmax_t __hard_link_count(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    struct ::stat st;
    detail::posix_stat(p, st, &m_ec);
    if (ec) *ec = m_ec;
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::hard_link_count", p, m_ec);
    } 
    else if (m_ec && ec) {
        return static_cast<std::uintmax_t>(-1);
    } else {
        return static_cast<std::uintmax_t>(st.st_nlink);
    }
}

////////////////////////////////////////////////////////////////////////////
bool __fs_is_empty(const path& p, std::error_code *ec)
{
    auto is_dir = is_directory(__status(p, ec));
    return (is_dir ? directory_iterator(p) == directory_iterator()
      : __file_size(p, ec) == 0);
}

////////////////////////////////////////////////////////////////////////////
file_time_type __last_write_time(const path& p, std::error_code *ec)
{
    using Clock = std::chrono::system_clock;
    
    std::error_code m_ec;
    struct ::stat st;
    detail::posix_stat(p, st, &m_ec);
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::last_write_time", p, m_ec);
    } 
    else if (m_ec) {
        return file_time_type::min();
    } else {
        return Clock::from_time_t(
            static_cast<std::time_t>(st.st_mtime)
          );
    }
}

////////////////////////////////////////////////////////////////////////////
void __last_write_time(
    const path& p
  , file_time_type new_time
  , std::error_code *ec
  )
{
    using Clock = std::chrono::system_clock;
    
    std::error_code m_ec;
    struct ::stat st;
    detail::posix_stat(p, st, &m_ec);
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::last_write_time", p, m_ec);
    }
    else if (m_ec) {
        return;
    }
    
    ::utimbuf tbuf;
    tbuf.actime = st.st_atime;
    tbuf.modtime = Clock::to_time_t(new_time);
    
    if (::utime(p.c_str(), &tbuf) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::last_write_time", p, m_ec);
    }
}

////////////////////////////////////////////////////////////////////////////
void __permissions(const path& p, perms prms, std::error_code *ec)
{
    _LIBCPP_ASSERT(not (bool(perms::add_perms & prms) 
                 && bool(perms::remove_perms & prms)), "invalid perms mask");
      
    file_status st = __status(p, ec);
    if (!exists(st)) {
        if (ec && *ec) { return; }
        std::error_code m_ec(
            static_cast<int>(std::errc::no_such_file_or_directory)
          , std::system_category()
          );
        throw filesystem_error("std::experimental::filesystem::permissions", p, m_ec);
    }
      
    if (bool(perms::add_perms & prms)) {
        prms |= st.permissions();
    }
    else if (bool(perms::remove_perms & prms)) {
        prms = st.permissions() & ~prms;
    }
    
    auto const real_perms = detail::posix_convert_perms(prms);
    
# if defined(AT_SYMLINK_NOFOLLOW) && defined(AT_FDCWD) \
  && !defined(__linux__)
    const int flags = bool(perms::resolve_symlinks & prms)
                   ? 0 : AT_SYMLINK_NOFOLLOW;
                   
    if (::fchmodat(AT_FDCWD, p.c_str(), real_perms, flags) == -1) {
# else
    if (::chmod(p.c_str(), real_perms) == -1) {
# endif
        std::error_code m_ec = detail::capture_errno();
        if (ec) {
            *ec = m_ec;
            return;
        } else {
            throw filesystem_error("std::experimental::filesystem::permissions", p, m_ec);
        }
    } 
    if (ec) ec->clear();
}

////////////////////////////////////////////////////////////////////////////
path __read_symlink(const path& p, std::error_code *ec)
{
    char buff[PATH_MAX + 1];
    std::error_code m_ec;
    ::ssize_t ret;
    if ((ret = ::readlink(p.c_str(), buff, PATH_MAX)) == -1) {
        m_ec = detail::capture_errno();
    } else {
        _LIBCPP_ASSERT(ret <= PATH_MAX, "TODO");
        _LIBCPP_ASSERT(ret > 0, "TODO");
        buff[ret] = 0;
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::read_symlink", p, m_ec);
    }
    else if (m_ec) {
        return {};
    } else {
        return path(buff);
    }
}

////////////////////////////////////////////////////////////////////////////
bool __remove(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    if (::remove(p.c_str()) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::remove", p, m_ec);
    }
    return !m_ec;
}

namespace
{
    ////////////////////////////////////////////////////////////////////////
    std::uintmax_t remove_all_impl(
        path const & p, file_status const & st
    , std::error_code *ec)
    {
        static constexpr std::uintmax_t npos = static_cast<std::uintmax_t>(-1);
        std::uintmax_t count = 1;
        if (is_directory(st)) {
            for (directory_iterator it(p); it != directory_iterator(); ++it) {
                const file_status fst = __symlink_status(it->path(), ec);
                if (ec && *ec) {
                    return npos;
                }
                const std::uintmax_t other_count = 
                    remove_all_impl(it->path(), fst, ec);
                if ((ec && *ec) || other_count == npos) {
                    return npos;
                }
                count += other_count;
            }
        }
        const bool ret = __remove(p, ec);
        _LIBCPP_ASSERT(ret, "Remove should have succeeded"); ((void)ret);
        if (ec && *ec) {
            return npos;
        
        } else {
            return count;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
std::uintmax_t __remove_all(const path& p, std::error_code *ec)
{
    std::error_code mec;
    const file_status st = __symlink_status(p, &mec);
    if (not status_known(st)) {
        if (ec) {
            _LIBCPP_ASSERT(mec, "Expected error code");
            *ec = mec;
            return static_cast<std::uintmax_t>(-1);
        } 
        throw filesystem_error("std::experimental::filesystem::remove_all", p, mec);
    }
    
    if (exists(st)) {
        return remove_all_impl(p, st, ec);
    } else {
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////////
void __rename(const path& from, const path& to, std::error_code *ec)
{
    std::error_code m_ec;
    if (::rename(from.c_str(), to.c_str()) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
        
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::rename", from, to, m_ec);
    }
}

////////////////////////////////////////////////////////////////////////////
void __resize_file(const path& p, std::uintmax_t size, std::error_code *ec)
{
    std::error_code m_ec;
    if (::truncate(p.c_str(), static_cast<long>(size)) == -1) {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
    
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::resize_file", p, m_ec);
    }
}

////////////////////////////////////////////////////////////////////////////
space_info __space(const path& p, std::error_code *ec)
{
    std::error_code m_ec;
    struct statvfs m_svfs;
    //if we fail but don't throw
    if (::statvfs(p.c_str(), &m_svfs) == -1)  {
        m_ec = detail::capture_errno();
    }
    if (ec) *ec = m_ec;
    if (m_ec && not ec) {
        throw filesystem_error("std::experimental::filesystem::space", p, m_ec);
    }
    else if (m_ec) {
        space_info si;
        si.capacity = si.free = si.available = 
            static_cast<decltype(si.free)>(-1);
        return si;
    } else {
        space_info si;
        si.capacity =   m_svfs.f_blocks * m_svfs.f_frsize;
        si.free =       m_svfs.f_bfree  * m_svfs.f_frsize;
        si.available =  m_svfs.f_bavail * m_svfs.f_frsize;
        return si;
    }
}

////////////////////////////////////////////////////////////////////////////
file_status __status(const path& p, std::error_code *ec)
{
    struct stat m_stat;
    return detail::posix_stat(p, m_stat, ec);
}

////////////////////////////////////////////////////////////////////////////
file_status __symlink_status(const path& p, std::error_code *ec)
{
    struct stat m_stat;
    return detail::posix_lstat(p, m_stat, ec);
}

////////////////////////////////////////////////////////////////////////////
path __system_complete(const path& p, std::error_code *ec)
{
    if (ec) ec->clear();
    return absolute(p, current_path());
}

////////////////////////////////////////////////////////////////////////////
path __temp_directory_path(std::error_code *ec)
{
    const char* env_paths[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR"};
    path p("/tmp");
    std::error_code m_ec;
        
    for (auto & ep : env_paths) 
    {
        char const* ret = std::getenv(ep);
        if (ret) {
            p = ret;
            break;
        }
    }

    if (is_directory(p, m_ec)) {
        if (ec) ec->clear();
        return p;
    } 

    m_ec = std::error_code(
        static_cast<int>(std::errc::no_such_file_or_directory)
      , std::system_category()
      );
    
    if (not ec) {
        throw filesystem_error("std::experimental::filesystem::temp_directory_path", m_ec);
    } else {
        *ec = m_ec;
        return {};
    }
}

////////////////////////////////////////////////////////////////////////////////
//                      OPERATORS DEFINITION                                                    
////////////////////////////////////////////////////////////////////////////////

    
    //since the specification of absolute in the current specification
    // this implementation is designed after the sample implementation
    // using the sample table as a guide
    path absolute(const path& p, const path& base)
    {
      auto root_name = p.root_name();
      auto root_dir = p.root_directory();
    
      if (!root_name.empty() && !root_dir.empty())
        return p;
        
      auto abs_base = base.is_absolute() ? base : absolute(base);
      
      /* !has_root_name && !has_root_dir */
      if (root_name.empty() && root_dir.empty()) 
      {
        return abs_base / p;
      }
      else if (!root_name.empty()) /* has_root_name && !has_root_dir */
      {
        return  root_name / abs_base.root_directory()
                /
                abs_base.relative_path() / p.relative_path();
      }
      else /* !has_root_name && has_root_dir */
      {
        if (abs_base.has_root_name())
          return abs_base.root_name() / p;
        // else p is absolute,  return outside of block
      }
      
      _LIBCPP_ASSERT(p.is_absolute(), "path must be absolute");
      return p;
    }
  
_LIBCPP_END_NAMESPACE_EXPERIMENTAL_FILESYSTEM
