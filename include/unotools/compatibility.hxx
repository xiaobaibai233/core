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
#ifndef INCLUDED_UNOTOOLS_COMPATIBILITY_HXX
#define INCLUDED_UNOTOOLS_COMPATIBILITY_HXX

#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/uno/Any.hxx>
#include <unotools/options.hxx>
#include <unotools/unotoolsdllapi.h>
#include <rtl/ustring.hxx>
#include <memory>
#include <vector>

namespace com::sun::star::beans { struct PropertyValue; }
namespace osl { class Mutex; }

/*-************************************************************************************************************
    @descr  Struct to hold information about one compatibility entry
*//*-*************************************************************************************************************/
class SvtCompatibilityEntry
{
    public:
        /*-************************************************************************************************************
            @descr          The method SvtCompatibilityOptions::GetList() returns a list of property values.
                            Use follow enum class to separate values by names.
                            Sync it with sPropertyName in SvtCompatibilityEntry::getName()
        *//*-*************************************************************************************************************/
        enum class Index
        {
            /* Should be in the start. Do not remove it. */
            Name,
            Module,

            /* Editable list of compatibility options. */
            UsePrtMetrics,
            AddSpacing,
            AddSpacingAtPages,
            UseOurTabStops,
            NoExtLeading,
            UseLineSpacing,
            AddTableSpacing,
            UseObjectPositioning,
            UseOurTextWrapping,
            ConsiderWrappingStyle,
            ExpandWordSpace,
            ProtectForm,
            MsWordTrailingBlanks,
            SubtractFlysAnchoredAtFlys,
            EmptyDbFieldHidesPara,
            /// special entry: optcomp.cxx converts the other values to
            /// integers but not this one because it doesn't have its own
            /// checkbox, so keep it at the end!
            AddTableLineSpacing,

            /* Should be at the end. Do not remove it. */
            INVALID
        };

        SvtCompatibilityEntry();

        static OUString getName( const Index rIdx );

        static constexpr OUStringLiteral USER_ENTRY_NAME = u"_user";
        static constexpr OUStringLiteral DEFAULT_ENTRY_NAME = u"_default";

        static size_t getElementCount()
        {
            return static_cast<size_t>(Index::INVALID);
        }

        css::uno::Any getValue( const Index rIdx ) const
        {
            if ( static_cast<size_t>(rIdx) < getElementCount() )
            {
                return m_aPropertyValue[ static_cast<int>(rIdx) ];
            } else
            {
                /* Wrong index. */
                assert( false );
                return css::uno::Any();
            }
        }

        template<typename T>
        T getValue( const Index rIdx ) const
        {
            T aValue = T();

            if ( static_cast<size_t>(rIdx) < getElementCount() )
            {
                m_aPropertyValue[ static_cast<int>(rIdx) ] >>= aValue;
            } else
            {
                /* Wrong index. */
                assert( false );
            }

            return aValue;
        }

        void setValue( const Index rIdx, css::uno::Any const & rValue )
        {
            if ( static_cast<size_t>(rIdx) < getElementCount() )
            {
                m_aPropertyValue[ static_cast<int>(rIdx) ] = rValue;
            } else
            {
                /* Wrong index. */
                assert( false );
            }
        }

        template<typename T>
        void setValue( const Index rIdx, T rValue )
        {
            if ( static_cast<size_t>(rIdx) < getElementCount() )
            {
                m_aPropertyValue[ static_cast<int>(rIdx) ] = css::uno::Any(rValue);
            } else
            {
                /* Wrong index. */
                assert( false );
            }
        }

        bool isDefaultEntry() const
        {
            return m_bDefaultEntry;
        }

        void setDefaultEntry( bool rValue )
        {
            m_bDefaultEntry = rValue;
        }

    private:
        std::vector<css::uno::Any> m_aPropertyValue;
        bool                       m_bDefaultEntry;
};

/*-************************************************************************************************************
    @short          forward declaration to our private date container implementation
    @descr          We use these class as internal member to support small memory requirements.
                    You can create the container if it is necessary. The class which use these mechanism
                    is faster and smaller then a complete implementation!
*//*-*************************************************************************************************************/
class SvtCompatibilityOptions_Impl;

/*-************************************************************************************************************
    @short          collect information about dynamic menus
    @descr          Make it possible to configure dynamic menu structures of menus like "new" or "wizard".
    @devstatus      ready to use
*//*-*************************************************************************************************************/
class UNOTOOLS_DLLPUBLIC SvtCompatibilityOptions final : public utl::detail::Options
{
    public:
        SvtCompatibilityOptions();
        virtual ~SvtCompatibilityOptions() override;

        /*-****************************************************************************************************
            @short      append a new item
            @descr

            @seealso    method Clear()

            @param      "aItem"             SvtCompatibilityEntry
        *//*-*****************************************************************************************************/
        void AppendItem( const SvtCompatibilityEntry& aItem );

        /*-****************************************************************************************************
            @short      clear complete specified list
            @descr      Call this methods to clear the whole list.
        *//*-*****************************************************************************************************/
        void Clear();

        void SetDefault( SvtCompatibilityEntry::Index rIdx, bool rValue );
        bool GetDefault( SvtCompatibilityEntry::Index rIdx ) const;

        /*-****************************************************************************************************
            @short      return complete specified list
            @descr      Call it to get all entries of compatibility options.
                        We return a list of all nodes with its names and properties.
            @return     A list of compatibility options is returned.

            @onerror    We return an empty list.
        *//*-*****************************************************************************************************/
        std::vector< SvtCompatibilityEntry > GetList() const;

    private:
        std::shared_ptr<SvtCompatibilityOptions_Impl> m_pImpl;

        /*-****************************************************************************************************
            @short      return a reference to a static mutex
            @descr      These class is partially threadsafe (for de-/initialization only).
                        All access methods aren't safe!
                        We create a static mutex only for one ime and use at different times.
            @return     A reference to a static mutex member.
        *//*-*****************************************************************************************************/
        UNOTOOLS_DLLPRIVATE static osl::Mutex& GetOwnStaticMutex();
};

#endif // INCLUDED_UNOTOOLS_COMPATIBILITY_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
