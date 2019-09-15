/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iANModalLabelControls.h"

#include "iANModalObjects.h"

#include <QLabel>
#include <QPalette>
#include <QSlider>
#include <QCheckBox>
#include <QGridLayout>

namespace {
	inline void setLabelColor(QLabel* label, QColor color) {
		QPalette palette = label->palette();
		palette.setColor(label->backgroundRole(), color);
		label->setAutoFillBackground(true);
		label->setPalette(palette);
	}
	const static int SLIDER_MAX = 100;
	inline void setOpacity(QSlider* slider, float opacityF) {
		int opacity = opacityF * SLIDER_MAX;
		slider->setValue(opacity);
	}
	inline float getOpacity(QSlider* slider) {
		return ((float)slider->value()) / SLIDER_MAX;
	}
}

iANModalLabelControls::iANModalLabelControls(QWidget* parent) {
	setParent(parent);
	m_layout = new QGridLayout(this);
}

void iANModalLabelControls::updateTable(QList<iANModalLabel> labels) {
	int i;
	for (i = 0; i < labels.size(); i++) {
		auto label = labels[i];
		float opacity = containsLabel(i) ? getOpacity(m_rows[i].opacity) : label.opacity;
		insertLabel(i, label, opacity);
	}
	int size = i;
	for (; i < m_rows.size(); i++) {
		removeLabel(i);
	}
	m_labels.resize(size);
	m_rows.resize(size);
}

void iANModalLabelControls::insertLabel(int rowIndex, iANModalLabel label, float opacity) {
	if (rowIndex >= m_labels.size()) {
		m_labels.resize(rowIndex + 1);
		m_labels[rowIndex] = label;
		addRow(rowIndex, label, opacity);
	} else {
		m_labels[rowIndex] = label;
		updateRow(rowIndex, label);
	}
}

void iANModalLabelControls::removeLabel(int rowIndex) {
	assert(containsLabel(rowIndex));
	Row row = m_rows[rowIndex];
	if (row.row != -1) {
		m_layout->removeWidget(row.name);
		m_layout->removeWidget(row.color);
		m_layout->removeWidget(row.opacity);
	}
}

bool iANModalLabelControls::containsLabel(int rowIndex) {
	return rowIndex < m_rows.size();
}

float iANModalLabelControls::opacity(int row) {
	assert(containsLabel(row));
	return getOpacity(m_rows[row].opacity);
}

bool iANModalLabelControls::remover(int row) {
	assert(containsLabel(row));
	return m_rows[row].remover->isChecked();
}

void iANModalLabelControls::addRow(int rowIndex, iANModalLabel label, float opacity) {
	assert(rowIndex <= m_layout->rowCount());
	int labelId = label.id;

	QCheckBox* cbRemover = new QCheckBox("Exclude");
	connect(cbRemover, &QCheckBox::stateChanged, this, [this, labelId]() { emit labelRemoverStateChanged(labelId); });

	QLabel *lName = new QLabel(label.name);

	QLabel *lColor = new QLabel();
	setLabelColor(lColor, label.color);

	QSlider* sOpacity = new QSlider(Qt::Orientation::Horizontal);
	sOpacity->setMinimum(0);
	sOpacity->setMaximum(SLIDER_MAX);
	setOpacity(sOpacity, opacity);
	connect(sOpacity, &QSlider::valueChanged, this, [this, labelId]() { emit labelOpacityChanged(labelId); });

	m_layout->addWidget(cbRemover, rowIndex, REMOVER);
	m_layout->addWidget(lName, rowIndex, NAME);
	m_layout->addWidget(lColor, rowIndex, COLOR);
	m_layout->addWidget(sOpacity, rowIndex, OPACITY);

	if (rowIndex >= m_rows.size()) {
		m_rows.resize(rowIndex + 1);
	}
	m_rows[rowIndex] = Row(rowIndex, cbRemover, lName, lColor, sOpacity);
}

void iANModalLabelControls::updateRow(int rowIndex, iANModalLabel label) {
	assert(containsLabel(rowIndex));
	Row row = m_rows[rowIndex];
	row.name->setText(label.name);
	setLabelColor(row.color, label.color);
	setOpacity(row.opacity, label.opacity);
	row.remover->setChecked(label.remover);
}