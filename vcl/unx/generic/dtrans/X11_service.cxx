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

#include <unx/salinst.h>

#include "X11_clipboard.hxx"
#include <com/sun/star/lang/IllegalArgumentException.hpp>

using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::datatransfer::clipboard;
using namespace com::sun::star::awt;
using namespace x11;

Sequence< OUString > x11::X11Clipboard_getSupportedServiceNames()
{
    return { "com.sun.star.datatransfer.clipboard.SystemClipboard" };
}

Sequence< OUString > x11::Xdnd_getSupportedServiceNames()
{
    return { "com.sun.star.datatransfer.dnd.X11DragSource" };
}

Sequence< OUString > x11::Xdnd_dropTarget_getSupportedServiceNames()
{
    return { "com.sun.star.datatransfer.dnd.X11DropTarget" };
}

// We run unit tests in parallel, which is a problem when touching a shared resource
// the system clipboard, so rather use the dummy GenericClipboard.
// Note, cannot make this a global variable, because it might be initialised BEFORE the putenv() call in cppunittester.
static bool IsRunningUnitTest() { return getenv("LO_TESTNAME") != nullptr; }

css::uno::Reference< XInterface > X11SalInstance::CreateClipboard( const Sequence< Any >& arguments )
{
    if ( IsRunningUnitTest() )
        return SalInstance::CreateClipboard( arguments );

    SelectionManager& rManager = SelectionManager::get();
    css::uno::Sequence<css::uno::Any> mgrArgs(1);
    mgrArgs[0] <<= Application::GetDisplayConnection();
    rManager.initialize(mgrArgs);

    OUString sel;
    if (!arguments.hasElements()) {
        sel = "CLIPBOARD";
    } else if (arguments.getLength() != 1 || !(arguments[0] >>= sel)) {
        throw css::lang::IllegalArgumentException(
            "bad X11SalInstance::CreateClipboard arguments",
            css::uno::Reference<css::uno::XInterface>(), -1);
    }
    Atom nSelection = rManager.getAtom(sel);

    std::unordered_map< Atom, css::uno::Reference< XClipboard > >::iterator it = m_aInstances.find( nSelection );
    if( it != m_aInstances.end() )
        return it->second;

    css::uno::Reference<css::datatransfer::clipboard::XClipboard> pClipboard = X11Clipboard::create( rManager, nSelection );
    m_aInstances[ nSelection ] = pClipboard;

    return pClipboard;
}

css::uno::Reference< XInterface > X11SalInstance::CreateDragSource()
{
    if ( IsRunningUnitTest() )
        return SalInstance::CreateDragSource();

    return css::uno::Reference < XInterface >( static_cast<OWeakObject *>(new SelectionManagerHolder()) );
}

css::uno::Reference< XInterface > X11SalInstance::CreateDropTarget()
{
    if ( IsRunningUnitTest() )
        return SalInstance::CreateDropTarget();

    return css::uno::Reference < XInterface >( static_cast<OWeakObject *>(new DropTarget()) );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
