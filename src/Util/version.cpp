/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/version.h>
#include <Util/include/debug.h>

namespace version
{

    /* Major version X (X.y.z | X > 0). */
    const uint32_t CLIENT_MAJOR = 5;

    /* Minor version Y (x.Y.z | Y > 0). */
    const uint32_t CLIENT_MINOR = 1;

    /* Patch version Z (x.y.Z | Z > 0). */
    const uint32_t CLIENT_PATCH = 0;


    /* The version of the actual wallet client. */
    const uint32_t CLIENT_VERSION =
        get_version(CLIENT_MAJOR, CLIENT_MINOR, CLIENT_PATCH);


    /* Client Version Outputs. */
    const std::string CLIENT_NAME("Tritium++");
    const std::string CLIENT_DATE(__DATE__ " " __TIME__);

    /* The version number */
    const std::string CLIENT_VERSION_STRING =
        debug::safe_printstr(CLIENT_MAJOR, ".", CLIENT_MINOR, ".", CLIENT_PATCH);


    /* The interface used Qt, CLI, or Tritium) */
    #if defined QT_GUI
        const std::string CLIENT_INTERFACE("Qt");
    #elif defined TRITIUM_GUI
        const std::string CLIENT_INTERFACE("GUI");
    #else
        const std::string CLIENT_INTERFACE("CLI");
    #endif


    /* The database type used (Berkeley DB, or Lower Level Database) */
    #if defined USE_LLD
        const std::string CLIENT_DATABASE("[LLD]");
    #else
        const std::string CLIENT_DATABASE("[BDB]");
    #endif


    /* The Architecture (32-Bit, ARM 64, or 64-Bit) */
    #if defined x86
        const std::string BUILD_ARCH = "[x86]";
    #elif defined aarch64
        const std::string BUILD_ARCH = "[ARM aarch64]";
    #else
        const std::string BUILD_ARCH = "[x64]";
    #endif


    /* Our literal build string. */
    const std::string CLIENT_VERSION_BUILD_STRING
    (
          CLIENT_VERSION_STRING + "-rc2 "
        + CLIENT_NAME           + " "
        + CLIENT_INTERFACE      + " "
        + CLIENT_DATABASE + BUILD_ARCH
    );


	/* Overload to decompose a integer version into string value. */
	std::string version_string(const uint32_t nVersion)
	{
		/* Decompose our individual sub-versions. */
		const uint32_t nMajor = nVersion / 10000;
		const uint32_t nMinor = (nVersion - (nMajor * 10000)) / 100;
		const uint32_t nPatch = (nVersion - (nMajor * 10000) - (nMinor * 100));

		return debug::safe_printstr(nMajor, ".", nMinor, ".", nPatch);
	}
}
