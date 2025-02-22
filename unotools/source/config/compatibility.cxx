/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <unotools/compatibility.hxx>
#include <unotools/configitem.hxx>
#include <unotools/syslocale.hxx>
#include <tools/debug.hxx>
#include <sal/log.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <i18nlangtag/languagetag.hxx>

#include "itemholder1.hxx"

#include <algorithm>
#include <vector>

using namespace ::std;
using namespace ::utl;
using namespace ::osl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;

#define ROOTNODE_OPTIONS        "Office.Compatibility"
#define PATHDELIMITER           "/"
#define SETNODE_ALLFILEFORMATS  "AllFileFormats"

SvtCompatibilityEntry::SvtCompatibilityEntry()
    : m_aPropertyValue( SvtCompatibilityEntry::getElementCount() )
{
    /* Should be in the start. Do not remove it. */
    setValue<OUString>( Index::Name, OUString() );
    setValue<OUString>( Index::Module, OUString() );

    /* Editable list of default values. Sync it with the SvtCompatibilityEntry::Index enum class. */
    setValue<bool>( Index::UsePrtMetrics, false );
    setValue<bool>( Index::AddSpacing, false );
    setValue<bool>( Index::AddSpacingAtPages, false );
    setValue<bool>( Index::UseOurTabStops, false );
    setValue<bool>( Index::NoExtLeading, false );
    setValue<bool>( Index::UseLineSpacing, false );
    setValue<bool>( Index::AddTableSpacing, false );
    setValue<bool>( Index::UseObjectPositioning, false );
    setValue<bool>( Index::UseOurTextWrapping, false );
    setValue<bool>( Index::ConsiderWrappingStyle, false );
    setValue<bool>( Index::ExpandWordSpace, true );
    setValue<bool>( Index::ProtectForm, false );
    setValue<bool>( Index::MsWordTrailingBlanks, false );
    setValue<bool>( Index::SubtractFlysAnchoredAtFlys, false );
    setValue<bool>( Index::EmptyDbFieldHidesPara, true );
    setValue<bool>( Index::AddTableLineSpacing, false );

    setDefaultEntry( false );
}

OUString SvtCompatibilityEntry::getName( const Index rIdx )
{
    static const char* sPropertyName[] =
    {
        /* Should be in the start. Do not remove it. */
        "Name",
        "Module",

        /* Editable list of compatibility option names. Sync it with the SvtCompatibilityEntry::Index enum class. */
        "UsePrinterMetrics",
        "AddSpacing",
        "AddSpacingAtPages",
        "UseOurTabStopFormat",
        "NoExternalLeading",
        "UseLineSpacing",
        "AddTableSpacing",
        "UseObjectPositioning",
        "UseOurTextWrapping",
        "ConsiderWrappingStyle",
        "ExpandWordSpace",
        "ProtectForm",
        "MsWordCompTrailingBlanks",
        "SubtractFlysAnchoredAtFlys",
        "EmptyDbFieldHidesPara",
        "AddTableLineSpacing",
    };

    /* Size of sPropertyName array not equal size of the SvtCompatibilityEntry::Index enum class */
    assert( SAL_N_ELEMENTS(sPropertyName) == static_cast<int>( SvtCompatibilityEntry::getElementCount() ) );

    return OUString::createFromAscii( sPropertyName[ static_cast<int>(rIdx) ] );
}

/*-****************************************************************************************************************
    @descr  support simple menu structures and operations on it
****************************************************************************************************************-*/

/*-****************************************************************************************************
    @short      base implementation of public interface for "SvtCompatibilityOptions"!
    @descr      These class is used as static member of "SvtCompatibilityOptions" ...
                => The code exist only for one time and isn't duplicated for every instance!
*//*-*****************************************************************************************************/
class SvtCompatibilityOptions_Impl : public ConfigItem
{
    public:
        SvtCompatibilityOptions_Impl();
        virtual ~SvtCompatibilityOptions_Impl() override;

        void AppendItem( const SvtCompatibilityEntry& aItem );
        void Clear();

        void SetDefault( SvtCompatibilityEntry::Index rIdx, bool rValue );
        bool GetDefault( SvtCompatibilityEntry::Index rIdx ) const;

        const std::vector< SvtCompatibilityEntry > & GetOptions() const { return m_aOptions; }

        /*-****************************************************************************************************
            @short      called for notify of configmanager
            @descr      This method is called from the ConfigManager before the application ends or from the
                        PropertyChangeListener if the sub tree broadcasts changes. You must update your
                        internal values.

            @seealso    baseclass ConfigItem

            @param      "lPropertyNames" is the list of properties which should be updated.
        *//*-*****************************************************************************************************/
        virtual void Notify( const Sequence< OUString >& lPropertyNames ) override;

    private:
        virtual void ImplCommit() override;

        /*-****************************************************************************************************
            @short      return list of key names of our configuration management which represent one module tree
            @descr      These methods return the current list of key names! We need it to get needed values from our
                        configuration management and support dynamical menu item lists!
            @return     A list of configuration key names is returned.
        *//*-*****************************************************************************************************/
        Sequence< OUString > impl_GetPropertyNames( Sequence< OUString >& rItems );

    private:
        std::vector< SvtCompatibilityEntry > m_aOptions;
        SvtCompatibilityEntry                m_aDefOptions;
};

SvtCompatibilityOptions_Impl::SvtCompatibilityOptions_Impl() : ConfigItem( ROOTNODE_OPTIONS )
{
    // Get names and values of all accessible menu entries and fill internal structures.
    // See impl_GetPropertyNames() for further information.
    Sequence< OUString > lNodes;
    Sequence< OUString > lNames  = impl_GetPropertyNames( lNodes );
    Sequence< Any >      lValues = GetProperties( lNames );

    // Safe impossible cases.
    // We need values from ALL configuration keys.
    // Follow assignment use order of values in relation to our list of key names!
    DBG_ASSERT( !( lNames.getLength()!=lValues.getLength() ), "SvtCompatibilityOptions_Impl::SvtCompatibilityOptions_Impl()\nI miss some values of configuration keys!\n" );

    // Get names/values for new menu.
    // 4 subkeys for every item!
    bool bDefaultFound = false;
    sal_Int32 nDestStep    = 0;
    for ( const auto& rNode : std::as_const(lNodes) )
    {
        SvtCompatibilityEntry aItem;

        aItem.setValue<OUString>( SvtCompatibilityEntry::Index::Name, rNode );

        for ( int i = static_cast<int>(SvtCompatibilityEntry::Index::Module); i < static_cast<int>(SvtCompatibilityEntry::Index::INVALID); ++i )
        {
            aItem.setValue( SvtCompatibilityEntry::Index(i), lValues[ nDestStep ] );
            nDestStep++;
        }

        m_aOptions.push_back( aItem );

        if ( !bDefaultFound && aItem.getValue<OUString>( SvtCompatibilityEntry::Index::Name ) == SvtCompatibilityEntry::DEFAULT_ENTRY_NAME )
        {
            SvtSysLocale aSysLocale;
            css::lang::Locale aLocale = aSysLocale.GetLanguageTag().getLocale();
            if ( aLocale.Language == "zh" || aLocale.Language == "ja" || aLocale.Language == "ko" )
                aItem.setValue<bool>( SvtCompatibilityEntry::Index::ExpandWordSpace, false );

            m_aDefOptions = aItem;
            bDefaultFound = true;
        }
    }
}

SvtCompatibilityOptions_Impl::~SvtCompatibilityOptions_Impl()
{
    assert( !IsModified() ); // should have been committed
}

void SvtCompatibilityOptions_Impl::AppendItem( const SvtCompatibilityEntry& aItem )
{
    m_aOptions.push_back( aItem );

    // default item reset?
    if ( aItem.getValue<OUString>( SvtCompatibilityEntry::Index::Name ) == SvtCompatibilityEntry::DEFAULT_ENTRY_NAME )
        m_aDefOptions = aItem;

    SetModified();
}

void SvtCompatibilityOptions_Impl::Clear()
{
    m_aOptions.clear();

    SetModified();
}

void SvtCompatibilityOptions_Impl::SetDefault( SvtCompatibilityEntry::Index rIdx, bool rValue )
{
    /* Are not set Name and Module */
    assert( rIdx != SvtCompatibilityEntry::Index::Name && rIdx != SvtCompatibilityEntry::Index::Module );

    m_aDefOptions.setValue<bool>( rIdx, rValue );
}

bool SvtCompatibilityOptions_Impl::GetDefault( SvtCompatibilityEntry::Index rIdx ) const
{
    /* Are not set Name and Module */
    assert( rIdx != SvtCompatibilityEntry::Index::Name && rIdx != SvtCompatibilityEntry::Index::Module );

    return m_aDefOptions.getValue<bool>( rIdx );
}

void SvtCompatibilityOptions_Impl::Notify( const Sequence< OUString >& )
{
    SAL_WARN( "unotools.config", "SvtCompatibilityOptions_Impl::Notify() Not implemented yet! I don't know how I can handle a dynamical list of unknown properties ..." );
}

void SvtCompatibilityOptions_Impl::ImplCommit()
{
    // Write all properties!
    // Delete complete set first.
    ClearNodeSet( SETNODE_ALLFILEFORMATS );

    Sequence< PropertyValue > lPropertyValues( SvtCompatibilityEntry::getElementCount() - 1 );
    sal_uInt32 nNewCount = m_aOptions.size();
    for ( sal_uInt32 nItem = 0; nItem < nNewCount; ++nItem )
    {
        SvtCompatibilityEntry aItem = m_aOptions[ nItem ];
        OUString              sNode = SETNODE_ALLFILEFORMATS PATHDELIMITER + aItem.getValue<OUString>( SvtCompatibilityEntry::Index::Name ) + PATHDELIMITER;

        for ( int i = static_cast<int>(SvtCompatibilityEntry::Index::Module); i < static_cast<int>(SvtCompatibilityEntry::Index::INVALID); ++i )
        {
            lPropertyValues[ i - 1 ].Name  = sNode + SvtCompatibilityEntry::getName( SvtCompatibilityEntry::Index(i) );
            lPropertyValues[ i - 1 ].Value = aItem.getValue( SvtCompatibilityEntry::Index(i) );
        }

        SetSetProperties( SETNODE_ALLFILEFORMATS, lPropertyValues );
    }
}

Sequence< OUString > SvtCompatibilityOptions_Impl::impl_GetPropertyNames( Sequence< OUString >& rItems )
{
    // First get ALL names of current existing list items in configuration!
    rItems = GetNodeNames( SETNODE_ALLFILEFORMATS );

    // expand list to result list ...
    Sequence< OUString > lProperties( rItems.getLength() * ( SvtCompatibilityEntry::getElementCount() - 1 ) );

    sal_Int32 nDestStep    = 0;
    // Copy entries to destination and expand every item with 2 supported sub properties.
    for ( const auto& rItem : std::as_const(rItems) )
    {
        OUString sFixPath = SETNODE_ALLFILEFORMATS PATHDELIMITER + rItem + PATHDELIMITER;
        for ( int i = static_cast<int>(SvtCompatibilityEntry::Index::Module); i < static_cast<int>(SvtCompatibilityEntry::Index::INVALID); ++i )
        {
            lProperties[ nDestStep ] = sFixPath + SvtCompatibilityEntry::getName( SvtCompatibilityEntry::Index(i) );
            nDestStep++;
        }
    }

    // Return result.
    return lProperties;
}

namespace
{
    std::weak_ptr<SvtCompatibilityOptions_Impl> theOptions;
}

SvtCompatibilityOptions::SvtCompatibilityOptions()
{
    // Global access, must be guarded (multithreading!).
    MutexGuard aGuard( GetOwnStaticMutex() );

    m_pImpl = theOptions.lock();
    if ( !m_pImpl )
    {
        m_pImpl    = std::make_shared<SvtCompatibilityOptions_Impl>();
        theOptions = m_pImpl;
        ItemHolder1::holdConfigItem( EItem::Compatibility );
    }
}

SvtCompatibilityOptions::~SvtCompatibilityOptions()
{
    // Global access, must be guarded (multithreading!)
    MutexGuard aGuard( GetOwnStaticMutex() );
    m_pImpl.reset();
}

void SvtCompatibilityOptions::AppendItem( const SvtCompatibilityEntry& aItem )
{
    MutexGuard aGuard( GetOwnStaticMutex() );
    m_pImpl->AppendItem( aItem );
}

void SvtCompatibilityOptions::Clear()
{
    MutexGuard aGuard( GetOwnStaticMutex() );
    m_pImpl->Clear();
}

void SvtCompatibilityOptions::SetDefault( SvtCompatibilityEntry::Index rIdx, bool rValue )
{
    MutexGuard aGuard( GetOwnStaticMutex() );
    m_pImpl->SetDefault( rIdx, rValue );
}

bool SvtCompatibilityOptions::GetDefault( SvtCompatibilityEntry::Index rIdx ) const
{
    MutexGuard aGuard( GetOwnStaticMutex() );
    return m_pImpl->GetDefault( rIdx );
}

std::vector< SvtCompatibilityEntry > SvtCompatibilityOptions::GetList() const
{
    MutexGuard aGuard( GetOwnStaticMutex() );

    return m_pImpl->GetOptions();
}

namespace
{
    class theCompatibilityOptionsMutex : public rtl::Static<osl::Mutex, theCompatibilityOptionsMutex>{};
}

Mutex& SvtCompatibilityOptions::GetOwnStaticMutex()
{
    return theCompatibilityOptionsMutex::get();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
