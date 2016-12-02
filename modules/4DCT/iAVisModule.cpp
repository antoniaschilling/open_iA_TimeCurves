/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "pch.h"
#include "iAVisModule.h"
// vtk
#include <vtkRenderer.h>

iAVisModule::iAVisModule( )
	: m_enabled( false )
{ /* not implemented */ }

bool iAVisModule::isAttached( )
{
	return m_renderer != nullptr;
}

void iAVisModule::attachTo( vtkRenderer* renderer )
{
	if( renderer == nullptr ) return;
	m_renderer = renderer;
}

void iAVisModule::detach( )
{
	m_renderer = nullptr;
}

void iAVisModule::reset( )
{ /* not implemented */ }

void iAVisModule::enable( )
{
	if( !m_enabled && isAttached( ) )
	{
		show( );
		m_enabled = true;
	}
}

void iAVisModule::disable( )
{
	if( m_enabled && isAttached( ) )
	{
		hide( );
		m_enabled = false;
	}
}

bool iAVisModule::isEnabled( )
{
	return m_enabled;
}
