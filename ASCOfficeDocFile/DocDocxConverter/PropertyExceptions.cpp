﻿/*
 * (c) Copyright Ascensio System SIA 2010-2018
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at Lubanas st. 125a-25, Riga, Latvia,
 * EU, LV-1021.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "PropertyExceptions.h"

#include "../Common/FormatUtils.h"

namespace DocFileFormat
{
	PropertyExceptions::~PropertyExceptions()
	{
		RELEASEOBJECT( this->grpprl );
	}

	PropertyExceptions::PropertyExceptions(): grpprl(NULL)
	{
		this->grpprl = new std::list<SinglePropertyModifier>();
	}

	PropertyExceptions::PropertyExceptions( const std::list<SinglePropertyModifier>& grpprl )
	{
		this->grpprl = new std::list<SinglePropertyModifier>( grpprl );
	}

	PropertyExceptions::PropertyExceptions( unsigned char* bytes, int size, bool oldVersion ) : grpprl(NULL)
	{
		this->grpprl = new std::list<SinglePropertyModifier>();

		if ( ( bytes == NULL ) || ( size == 0 ) ) return;

		//read the sprms
	              
		int sprmStart = 0;
		bool goOn = true;

		int opCodeSize = (oldVersion ? 1 : 2);

		while ( goOn )
		{
			if ( ( sprmStart + opCodeSize ) < size )
			{
				unsigned short code  = oldVersion ? FormatUtils::BytesToUChar	( bytes, sprmStart, size ) :
													FormatUtils::BytesToUInt16	( bytes, sprmStart, size ) ;

				if (oldVersion && code == 0) 
				{
					sprmStart++;
					continue;
				}
				OperationCode opCode = (OperationCode)code;
				short opSize = -1;

				if (oldVersion)
				{
					opSize = (short)SinglePropertyModifier::GetOldOperandSize( (unsigned char)opCode );
				}
				else
				{
					unsigned char spra = (unsigned char)( (int)opCode >> 13 ); 
					opSize = (short)SinglePropertyModifier::GetOperandSize( spra );
				}
				unsigned char lenByte = 0;

				//operand has variable size
				if ( opSize == 255 )
				{
					//some opCode need special treatment
					switch ( opCode )
					{
						case sprmOldTDefTable:
						case sprmOldTDefTable10:
						case sprmTDefTable:
						case sprmTDefTable10:
						{
							//The opSize of the table definition is stored in 2 bytes instead of 1
							lenByte = 2;
							opSize = FormatUtils::BytesToInt16( bytes, ( sprmStart + opCodeSize ), size );
							//Word adds an additional unsigned char to the opSize to compensate the additional
							//unsigned char needed for the length
							opSize--;
						}break;				  
				      
						case sprmOldPChgTabs:
						case sprmPChgTabs:
						{
							//The tab operand can be bigger than 255 bytes (length unsigned char is set to 255).
							//In this case a special calculation of the opSize is needed
							lenByte = 1;
							opSize = bytes[sprmStart + opCodeSize];

							if ( opSize == 255 )
							{
							  unsigned char itbdDelMax = bytes[sprmStart + opCodeSize + 1];
							  unsigned char itbdAddMax = bytes[sprmStart + opCodeSize + 1 + 2 * itbdDelMax];
							  
							  opSize = (short)( ( itbdDelMax * 4 + itbdAddMax * 3 ) - 1 );
							}
						}break;	

						case sprmOldPAnld:
							{
								lenByte = 1;
								opSize	= bytes[sprmStart + opCodeSize];
							}break;
				      
						default:
						{  
							//The variable length stand in the unsigned char after the opcode
							lenByte = 1;
							opSize = bytes[sprmStart + opCodeSize];
						}
						break;
					}
				}

				//length is 2byte for the opCode, lenByte for the length, opSize for the length of the operand
				int sprmBytesSize = opCodeSize + lenByte + opSize;
				unsigned char* sprmBytes = NULL;

				sprmBytes = new unsigned char[sprmBytesSize];

				if ( size >= ( sprmStart + sprmBytesSize ) )
				{
					memcpy( sprmBytes, ( bytes + sprmStart ), sprmBytesSize );

					SinglePropertyModifier sprm( sprmBytes, sprmBytesSize, oldVersion );
					grpprl->push_back( sprm );

					sprmStart += sprmBytesSize;
				}
				else
				{
					goOn = false;
				}

				RELEASEARRAYOBJECTS( sprmBytes );
			}
			else
			{
				goOn = false;
			}		
		}
	}

}