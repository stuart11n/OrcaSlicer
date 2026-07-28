#include <catch2/catch_all.hpp>

#include "libslic3r/Utils.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#ifndef _WIN32
#include <unistd.h>     // getuid
#endif

using namespace Slic3r;

TEST_CASE("per_user_temp_dir composes a per-user temp root", "[utils]") {
    const std::string base = "/tmp";

    SECTION("an empty id returns base unchanged") {
        REQUIRE(per_user_temp_dir(base, "") == base);
    }
    SECTION("a non-empty id is appended at the top level") {
        REQUIRE(per_user_temp_dir(base, "1000") == base + "/orcaslicer_1000");
    }
    SECTION("distinct ids produce distinct roots") {
        REQUIRE(per_user_temp_dir(base, "1000") != per_user_temp_dir(base, "1001"));
    }
}

TEST_CASE("per_user_temp_id follows the platform contract", "[utils]") {
    const std::string id = per_user_temp_id();

    SECTION("stable across calls") {
        REQUIRE(per_user_temp_id() == id);
    }
#ifdef _WIN32
    SECTION("empty on Windows (its temp dir is already per-user)") {
        REQUIRE(id.empty());
    }
#else
    SECTION("the current uid on Linux/macOS") {
        REQUIRE_FALSE(id.empty());
        REQUIRE(id == std::to_string(static_cast<unsigned long>(::getuid())));
    }
#endif
}

// The end-to-end contract callers depend on: the temp root is left alone on
// Windows and isolated per user on Linux/macOS.
TEST_CASE("per-user temp root is unchanged on Windows, isolated elsewhere", "[utils]") {
    const std::string base = "/tmp";
    const std::string root = per_user_temp_dir(base, per_user_temp_id());
#ifdef _WIN32
    REQUIRE(root == base);
#else
    REQUIRE(root != base);
    REQUIRE_THAT(root, Catch::Matchers::StartsWith(base + "/orcaslicer_"));
#endif
}

TEST_CASE("normalize_data_dir_for_instance_hash treats equivalent paths as one identity", "[utils][InstanceCheck]") {
    namespace fs = boost::filesystem;
    const fs::path base = fs::temp_directory_path() / "orca_instance_hash_test";
    fs::create_directories(base);
    const fs::path dir_a = base / "profile_a";
    const fs::path dir_b = base / "profile_b";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);

    const std::string abs_a = fs::absolute(dir_a).string();
    const std::string with_slash = abs_a + "/";
    const std::string via_dot = (dir_a / "." ).string();

    const std::string hash_a = normalize_data_dir_for_instance_hash(abs_a);
    REQUIRE(hash_a == normalize_data_dir_for_instance_hash(with_slash));
    REQUIRE(hash_a == normalize_data_dir_for_instance_hash(via_dot));
    REQUIRE(hash_a != normalize_data_dir_for_instance_hash(dir_b.string()));
    REQUIRE_FALSE(hash_a.empty());

    fs::remove_all(base);
}
