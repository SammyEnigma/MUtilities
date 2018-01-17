///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

/***************************************************************************
**                                                                        **
**  QKeccakHash, an API wrapper bringing the optimized implementation of  **
**  Keccak (http://keccak.noekeon.org/) to Qt.                            **
**  Copyright (C) 2013 Emanuel Eichhammer                                 **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Emanuel Eichhammer                                   **
**  Website/Contact: http://www.WorksLikeClockwork.com/                   **
**             Date: 12.01.12                                             **
****************************************************************************/

#include "Hash_Keccak.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDebug>

//Implementation
#include "3rd_party/keccak/include/keccak_impl.h"

MUtils::Hash::Keccak::Keccak()
{
	m_initialized = false;
	m_state = (MUtils::Hash::Internal::KeccakImpl::hashState*) _aligned_malloc(sizeof(MUtils::Hash::Internal::KeccakImpl::hashState), 32);
	if(!m_state)
	{
		MUTILS_THROW("_aligned_malloc() has failed, probably out of heap space!");
	}
	memset(m_state, 0, sizeof(MUtils::Hash::Internal::KeccakImpl::hashState));
}

MUtils::Hash::Keccak::~Keccak()
{
	if(m_state)
	{
		_aligned_free(m_state);
		m_state = NULL;
	}
}

bool MUtils::Hash::Keccak::init(const HashBits hashBits)
{
	if(m_initialized)
	{
		qWarning("MUtils::KeccakHash has already been initialized!");
		return false;
	}

	memset(m_state, 0, sizeof(MUtils::Hash::Internal::KeccakImpl::hashState));
	int hashBitLength = 0;

	switch (hashBits)
	{
		case hb224: hashBitLength = 224; break;
		case hb256: hashBitLength = 256; break;
		case hb384: hashBitLength = 384; break;
		case hb512: hashBitLength = 512; break;
		default: throw "Invalid hash length!!";
	}

	if(MUtils::Hash::Internal::KeccakImpl::Init(m_state, hashBitLength) != MUtils::Hash::Internal::KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Init() has failed unexpectedly!");
		return false;
	}
	
	m_initialized = true;
	return true;
}


bool MUtils::Hash::Keccak::process(const quint8 *const data, const quint32 len)
{
	if(!m_initialized)
	{
		qWarning("MUtils::KeccakHash has not been initialized yet!");
		return false;
	}
	
	if(MUtils::Hash::Internal::KeccakImpl::Update(m_state, (MUtils::Hash::Internal::KeccakImpl::BitSequence*)data, len*8U) != MUtils::Hash::Internal::KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Update() has failed unexpectedly!");
		m_initialized = false;
		return false;
	}
	
	return true;
}

QByteArray MUtils::Hash::Keccak::finalize(void)
{
	if(!m_initialized)
	{
		qWarning("MUtils::KeccakHash has not been initialized yet!");
		return QByteArray();
	}

	Q_ASSERT(m_state->fixedOutputLength > 0);
	Q_ASSERT((m_state->fixedOutputLength % 8) == 0);

	QByteArray hashResult(m_state->fixedOutputLength / 8, '\0');
	if(MUtils::Hash::Internal::KeccakImpl::Final(m_state, (MUtils::Hash::Internal::KeccakImpl::BitSequence*)hashResult.data()) != MUtils::Hash::Internal::KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Final() has failed unexpectedly!");
		hashResult.clear();
	}

	m_initialized = false;
	return hashResult;
}

MUtils::Hash::Keccak *MUtils::Hash::Keccak::create(const HashBits hashBit, const char *const key)
{
	Keccak *const keccak = new Keccak();
	if (!keccak->init(hashBit))
	{
		MUTILS_THROW("Keccak initialization has failed!");
	}
	if (key)
	{
		keccak->update(((const Internal::KeccakImpl::UINT8*)key), strlen(key));
	}
	return keccak;
}

bool MUtils::Hash::Keccak::selfTest(void)
{
	Keccak hash;
	const QByteArray input("The quick brown fox jumps over the lazy dog");
	bool passed[4] = {false, false, false, false};


	if(hash.init(MUtils::Hash::Keccak::hb224))
	{
		if(hash.update(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[0] = (_stricmp(result.toHex().constData(), "310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe") == 0);
				if(!passed[0]) qWarning("MUtils::KeccakHash self-test: Test #1 failed !!!");
			}
		}
	}

	if(hash.init(MUtils::Hash::Keccak::hb256))
	{
		if(hash.update(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[1] = (_stricmp(result.toHex().constData(), "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15") == 0);
				if(!passed[1]) qWarning("MUtils::KeccakHash self-test: Test #2 failed !!!");
			}
		}
	}
	
	if(hash.init(MUtils::Hash::Keccak::hb384))
	{
		if(hash.update(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[2] = (_stricmp(result.toHex().constData(), "283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3") == 0);
				if(!passed[2]) qWarning("MUtils::KeccakHash self-test: Test #3 failed !!!");
			}
		}
	}

	if(hash.init(MUtils::Hash::Keccak::hb512))
	{
		if(hash.update(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[3] = (_stricmp(result.toHex().constData(), "d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609") == 0);
				if(!passed[3]) qWarning("MUtils::KeccakHash self-test: Test #4 failed !!!");
			}
		}
	}

	return (passed[0] && passed[1] && passed[2] && passed[3]);
}
