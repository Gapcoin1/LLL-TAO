/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "act the way you'd like to be and soon you'll be the way you act" - Leonard Cohen

____________________________________________________________________________________________*/


#include <TAO/Register/types/object.h>
#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/include/timelocks.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Default constructor. */
        Object::Object()
        : State     (uint8_t(REGISTER::OBJECT))
        , vchSystem (512, 0) //system memory by default is 512 bytes
        , mapData   ()
        {
        }


        /* Copy Constructor. */
        Object::Object(const Object& object)
        : State     (object)
        , vchSystem (object.vchSystem)
        , mapData   (object.mapData)
        {
        }


        /* Move Constructor. */
        Object::Object(Object&& object) noexcept
        : State     (std::move(object))
        , vchSystem (std::move(object.vchSystem))
        , mapData   (std::move(object.mapData))
        {
        }


        /* Assignment operator overload */
        Object& Object::operator=(const Object& object)
        {
            vchState     = object.vchState;

            nVersion     = object.nVersion;
            nType        = object.nType;
            hashOwner    = object.hashOwner;
            nCreated     = object.nCreated;
            nModified    = object.nModified;
            hashChecksum = object.hashChecksum;

            nReadPos     = 0; //don't copy over read position
            mapData      = object.mapData;

            return *this;
        }


        /* Assignment operator overload */
        Object& Object::operator=(Object&& object) noexcept
        {
            vchState     = std::move(object.vchState);

            nVersion     = std::move(object.nVersion);
            nType        = std::move(object.nType);
            hashOwner    = std::move(object.hashOwner);
            nCreated     = std::move(object.nCreated);
            nModified    = std::move(object.nModified);
            hashChecksum = std::move(object.hashChecksum);

            nReadPos     = 0; //don't copy over read position
            mapData      = std::move(object.mapData);

            return *this;
        }


        /* Default Destructor */
        Object::~Object()
        {
        }


        /* Copy Constructor. */
        Object::Object(const State& state)
        : State     (state)
        , vchSystem ()
        , mapData   ()
        {
        }


        /* Get's the standard object type. */
        uint8_t Object::Standard() const
        {
            /* Default short circuit for state registers being loaded as object. */
            if(nType != REGISTER::OBJECT)
                return nType;

            /* Set the return value. */
            uint8_t nStandard =
                OBJECTS::NONSTANDARD;

            /* Search object register for key types. */
            if(mapData.size() == 1
            && Check("namespace", TYPES::STRING, false))
            {
                /* If it only contains one field called namespace then it must be a namespace */
                /* Set the return value. */
                nStandard = OBJECTS::NAMESPACE;

            }
            else if(mapData.size() == 9
            && Check("auth",    TYPES::UINT256_T, true)
            && Check("lisp",    TYPES::UINT256_T, true)
            && Check("network", TYPES::UINT256_T, true)
            && Check("sign",    TYPES::UINT256_T, true)
            && Check("verify",  TYPES::UINT256_T, true)
            && Check("cert",    TYPES::UINT256_T, true)
            && Check("app1",    TYPES::UINT256_T, true)
            && Check("app2",    TYPES::UINT256_T, true)
            && Check("app3",    TYPES::UINT256_T, true))
            {
                /* Set the return value. */
                nStandard = OBJECTS::CRYPTO;
            }
            else if(mapData.size() == 3
            && Check("namespace", TYPES::STRING, false)
            && Check("name",      TYPES::STRING, false)
            && Check("address")) /* Name registers can store different types in the address so don't check the field type */
            {
                /* Set the return value. */
                nStandard = OBJECTS::NAME;

            }
            else if(Check("token", TYPES::UINT256_T, false)
            && Check("balance",    TYPES::UINT64_T,  true))
            {
                /* Set the return value. */
                nStandard = OBJECTS::ACCOUNT;

                /* Make the supply immutable for now (add continued distribution later). */
                if(Check("supply",   TYPES::UINT64_T, false)
                && Check("decimals", TYPES::UINT8_T, false))
                {
                    /* Set the return value. */
                    nStandard = OBJECTS::TOKEN;
                }
                else if(Check("trust", TYPES::UINT64_T, true)
                     && Check("stake", TYPES::UINT64_T, true))
                {
                    /* Set the return value. */
                    nStandard = OBJECTS::TRUST;
                }
            }

            return nStandard;
        }


        /* Get's the standard object base type. */
        uint8_t Object::Base() const
        {
            /* Default short circuit for state registers being loaded as object. */
            if(nType != REGISTER::OBJECT)
                return nType;

            /* Set the return value. */
            uint8_t nStandard =
                OBJECTS::NONSTANDARD;

            /* Search object register for key types. */
            if(Check("token",   TYPES::UINT256_T, false)
            && Check("balance", TYPES::UINT64_T,  true))
            {
                /* Set the return value. */
                nStandard = OBJECTS::ACCOUNT;
            }
            else if(Check("namespace", TYPES::STRING, false))
            {
                /* Set the return value. */
                nStandard = OBJECTS::NAMESPACE;
            }

            return nStandard;
        }


        /* Get the cost to create this object register.*/
        uint64_t Object::Cost() const
        {
            /* Check the map for empty. */
            if(mapData.empty())
                throw debug::exception(FUNCTION, "cannot get cost when object isn't parsed");

            /* Switch based on standard types. */
            uint8_t nStandard = Standard();
            switch(nStandard)
            {
                /* Token fees are based on the total tokens created. */
                case OBJECTS::TOKEN:
                {
                    /* Get the supply from the token object */
                    uint64_t nSupply = get<uint64_t>("supply");

                    /* Fee = (log10(nSupply) - 2) * 100 NXS
                       which equates to 100 NXS for each significant figure, with the first 2sf (100 supply) being 1 NXS*/
                    uint64_t nBase = (std::log10(nSupply));
                    return std::max(int64_t(MIN_TOKEN_FEE),  int64_t(std::max(int64_t(0), int64_t(nBase - 2)) * TOKEN_FEE));
                }

                /* Name objects have specific fees. */
                case OBJECTS::NAME:
                {
                    /* Global names cost 2000 NXS */
                    if(get<std::string>("namespace") == NAMESPACE::GLOBAL)
                        return GLOBAL_NAME_FEE;

                    /* Local names cost 1 NXS. */
                    else
                        return NAME_FEE;
                }

                /* Namespaces cost 1000 NXS. */
                case OBJECTS::NAMESPACE:
                    return NAMESPACE_FEE;

                /* Handle for Account Fees. */
                case OBJECTS::ACCOUNT:
                case OBJECTS::TRUST:
                    return ACCOUNT_FEE;

                /* Crypto object registers have special fees. */
                case OBJECTS::CRYPTO:
                    return CRYPTO_FEE;

                /* non standard object cost is dependant on the data size. */
                case OBJECTS::NONSTANDARD:
                {
                    /* The fee changed with transaction version 2 so need to apply version-dependent fee. NOTE we can use the
                       nCreated time to determine the transaction version, as this is set to the transaction time when it is
                       first created.  */
                    const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
                    if(nCurrent < 2 || (nCurrent == 2 && !TAO::Ledger::TransactionVersionActive(nCreated, 2)))
                        return std::max(MIN_DATA_FEE, vchState.size() * DATA_FEE_V1);
                    else
                        return std::max(MIN_DATA_FEE, vchState.size() * DATA_FEE); //TODO: this is redundant, MIN_DATA_FEE wasn't lowered
                }

                default:
                    return MIN_DATA_FEE;
            }

            return MIN_DATA_FEE;
        }


        /* Parses out the data members of an object register. */
        bool Object::Parse() const
        {
            /* Ensure that object register is of proper type. */
            if(this->nType != REGISTER::OBJECT
            && this->nType != REGISTER::SYSTEM)
                return debug::error(FUNCTION, "register has invalid type ", uint32_t(this->nType));

            /* Check the map for empty. */
            if(!mapData.empty())
                return false;

            /* Reset the read position. */
            nReadPos   = 0;

            /* Read until end of state. */
            while(!end())
            {
                /* Deserialize the named value. */
                std::string strName;
                *this >> strName;

                /* Disallow duplicate value entries. */
                if(mapData.count(strName))
                    return debug::error(FUNCTION, "duplicate value entries");

                /* Deserialize the type. */
                uint8_t nCode;
                *this >> nCode;

                /* Mutable default: false (read only). */
                bool fMutable = false;

                /* Check for mutable specifier. */
                if(nCode == TYPES::MUTABLE)
                {
                    /* Set this type to be mutable. */
                    fMutable = true;

                    /* If mutable found, deserialize the type. */
                    *this >> nCode;
                }

                /* Switch between supported types. */
                switch(nCode)
                {

                    /* Standard type for C++ uint8_t. */
                    case TYPES::UINT8_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 2;

                        break;
                    }


                    /* Standard type for C++ uint16_t. */
                    case TYPES::UINT16_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 3;

                        break;
                    }


                    /* Standard type for C++ uint32_t. */
                    case TYPES::UINT32_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 5;

                        break;
                    }


                    /* Standard type for C++ uint64_t. */
                    case TYPES::UINT64_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 9;

                        break;
                    }


                    /* Standard type for Custom uint256_t */
                    case TYPES::UINT256_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 33;

                        break;
                    }


                    /* Standard type for Custom uint512_t */
                    case TYPES::UINT512_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 65;

                        break;
                    }


                    /* Standard type for Custom uint1024_t */
                    case TYPES::UINT1024_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 129;

                        break;
                    }


                    /* Standard type for STL string */
                    case TYPES::STRING:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate to start of size. */
                        ++nReadPos;

                        /* Find the serialized size of type. */
                        const uint64_t nSize =
                            ReadCompactSize(*this);

                        /* Iterate the type size */
                        nReadPos += nSize;

                        break;
                    }


                    /* Standard type for STL vector with C++ type uint8_t */
                    case TYPES::BYTES:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(strName, std::make_pair(--nReadPos, fMutable));

                        /* Iterate to start of size. */
                        ++nReadPos;

                        /* Find the serialized size of type. */
                        const uint64_t nSize =
                            ReadCompactSize(*this);

                        /* Iterate the type size */
                        nReadPos += nSize;

                        break;
                    }


                    /* Fail if types are unknown. */
                    default:
                        return debug::error(FUNCTION, "malformed object register (unexpected instruction ", uint32_t(nCode), ")");
                }
            }

            return true;
        }


        /* Get a list of field names for this Object. */
        std::vector<std::string> Object::Members() const
        {
            /* Declare the vector of field names to return */
            std::vector<std::string> vFieldNames;

            /* Check the map for empty. */
            if(mapData.empty()) //TODO: this should either throw, or this method should return by reference
                throw debug::exception(FUNCTION, "object is not parsed");

            /* Iterate data map and pull field names out into return vector */
            for(const auto& fieldName : mapData)
                vFieldNames.push_back(fieldName.first);

            return vFieldNames;
        }


        /* Get the type enumeration from the object register. */
        bool Object::Type(const std::string& strName, uint8_t& nType) const
        {
            /* Check for non-objects. */
            if(this->nType != TAO::Register::REGISTER::OBJECT)
                return false;

            /* Check the map for empty. */
            if(mapData.empty())
                return false;

            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            *this >> nType;

            /* Check for unsupported type enums. */
            if(nType == TYPES::UNSUPPORTED)
                return debug::error(FUNCTION, "unsupported type");

            return true;
        }


        /* Check the type enumeration from the object register. */
        bool Object::Check(const std::string& strName, const uint8_t nType, bool fMutable) const
        {
            /* Check for non-objects. */
            if(this->nType != TAO::Register::REGISTER::OBJECT)
                return false;

            /* Check the map for empty. */
            if(mapData.empty())
                return false;

            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            uint8_t nCheck;
            *this >> nCheck;

            /* Check for unsupported type enums. */
            if(nType != nCheck)
                return false;

            return (fMutable == mapData[strName].second);
        }


        /* Check that given field name exists in the object. */
        bool Object::Check(const std::string& strName) const
        {
            /* Check for non-objects. */
            if(this->nType != TAO::Register::REGISTER::OBJECT)
                return false;

            /* Check the map for empty. */
            if(mapData.empty())
                return false;

            /* Check that the name exists in the object. */
            return mapData.count(strName) > 0;
        }


        /*  Get the size of value in object register. */
        uint64_t Object::Size(const std::string& strName) const
        {
            /* Check for non-objects. */
            if(this->nType != TAO::Register::REGISTER::OBJECT)
                return false;

            /* Check the map for empty. */
            if(mapData.empty())
                return false;

            /* Get the type for given name. */
            uint8_t nType;
            if(!Type(strName, nType))
                return 0;

            /* Switch between supported types. */
            switch(nType)
            {

                /* Standard type for C++ uint8_t. */
                case TYPES::UINT8_T:
                    return 1;

                /* Standard type for C++ uint16_t. */
                case TYPES::UINT16_T:
                    return 2;

                /* Standard type for C++ uint32_t. */
                case TYPES::UINT32_T:
                    return 4;

                /* Standard type for C++ uint64_t. */
                case TYPES::UINT64_T:
                    return 8;

                /* Standard type for Custom uint256_t */
                case TYPES::UINT256_T:
                    return 32;

                /* Standard type for Custom uint512_t */
                case TYPES::UINT512_T:
                    return 64;

                /* Standard type for Custom uint1024_t */
                case TYPES::UINT1024_T:
                    return 128;

                /* Standard type for STL string */
                case TYPES::STRING:
                    return ReadCompactSize(*this);

                /* Standard type for STL vector with C++ type uint8_t */
                case TYPES::BYTES:
                    return ReadCompactSize(*this);
            }

            return 0;
        }


        /* Write into the object register a value of type bytes. */
        bool Object::Write(const std::string& strName, const std::string& strValue)
        {
            /* Check the map for empty. */
            if(mapData.empty())
                return debug::error(FUNCTION, "object is not parsed");

            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Check that the value is mutable (writes allowed). */
            if(!mapData[strName].second)
                return debug::error(FUNCTION, "cannot set value for READONLY data member");

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            uint8_t nType;
            *this >> nType;

            /* Make sure that value being written is type-safe. */
            if(nType != TYPES::STRING)
                return debug::error(FUNCTION, "type must be string");

            /* Get the expected size. */
            uint64_t nSize = ReadCompactSize(*this);
            if(nSize != strValue.size())
                return debug::error(FUNCTION, "string size mismatch");

            /* Check for memory overflows. */
            if(nReadPos + nSize > vchState.size())
                return debug::error(FUNCTION, "performing an over-write");

            /* Copy the bytes into the object. */
            std::copy((uint8_t*)&strValue[0], (uint8_t*)&strValue[0] + strValue.size(), (uint8_t*)&vchState[nReadPos]);

            return true;
        }


        /* Write into the object register a value of type bytes. */
        bool Object::Write(const std::string& strName, const std::vector<uint8_t>& vData)
        {
            /* Check the map for empty. */
            if(mapData.empty())
                return debug::error(FUNCTION, "object is not parsed");

            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Check that the value is mutable (writes allowed). */
            if(!mapData[strName].second)
                return debug::error(FUNCTION, "cannot set value for READONLY data member");

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            uint8_t nType;
            *this >> nType;

            /* Make sure that value being written is type-safe. */
            if(nType != TYPES::BYTES)
                return debug::error(FUNCTION, "type must be bytes");

            /* Get the expected size. */
            uint64_t nSize = ReadCompactSize(*this);
            if(nSize != vData.size())
                return debug::error(FUNCTION, "bytes size mismatch");

            /* Check for memory overflows. */
            if(nReadPos + nSize > vchState.size())
                return debug::error(FUNCTION, "performing an over-write");

            /* Copy the bytes into the object. */
            std::copy((uint8_t*)&vData[0], (uint8_t*)&vData[0] + vData.size(), (uint8_t*)&vchState[nReadPos]);

            return true;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint8_t n) const
        {
            return TYPES::UINT8_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint16_t n) const
        {
            return TYPES::UINT16_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint32_t n) const
        {
            return TYPES::UINT32_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint64_t& n) const
        {
            return TYPES::UINT64_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint256_t& n) const
        {
            return TYPES::UINT256_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint512_t& n) const
        {
            return TYPES::UINT512_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint1024_t& n) const
        {
            return TYPES::UINT1024_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const std::string& n) const
        {
            return TYPES::STRING;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const std::vector<uint8_t>& n) const
        {
            return TYPES::BYTES;
        }
    }
}
