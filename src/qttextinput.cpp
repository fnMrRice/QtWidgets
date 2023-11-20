#include "qttextinput.h"
#include "qttextinput_p.h"
#include <QEvent>
#include <QPainter>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QDebug>

FNRICE_QT_WIDGETS_BEGIN_NAMESPACE

QtTextInput::QtTextInput(QWidget *parent)
    : QWidget(parent), d_ptr(new QtTextInputPrivate(this)) {
    Q_D(QtTextInput);

    this->setLayout(d->main_layout);
    this->setCursor(Qt::IBeamCursor);

    connect(d->line_edit, &QLineEdit::textEdited, this, &QtTextInput::textEdited);
    connect(d->line_edit, &QLineEdit::textChanged, this, &QtTextInput::textChanged);
    connect(d->line_edit, &QLineEdit::returnPressed, this, &QtTextInput::returnPressed);
}

QtTextInput::~QtTextInput() {
    Q_D(QtTextInput);
    delete d_ptr;
}

void QtTextInput::setBorderWidth(int width) {
    Q_D(QtTextInput);
    d->border_width = width;
    this->update();
}

void QtTextInput::setBorderStyle(Qt::PenStyle style) {
    Q_D(QtTextInput);
    if (d->border_style == style) return;
    d->border_style = style;
    if (style == Qt::NoPen) {
        d->input_layout->setContentsMargins(NO_BORDER_INPUT_MARGINS);
    } else {
        d->input_layout->setContentsMargins(WITH_BORDER_INPUT_MARGINS);
    }
    this->update();
}

void QtTextInput::setBorderRadius(int radius) {
    Q_D(QtTextInput);
    d->border_radius = radius;
}

void QtTextInput::setBorderColor(QtTextInput::InputState state, const QColor &color) {
    Q_D(QtTextInput);
    switch (state) {
        case QtTextInput::StateNormal:
            d->border_color.normal = color;
            break;
        case QtTextInput::StateDisabled:
            d->border_color.disabled = color;
            break;
        case QtTextInput::StateFocus:
            d->border_color.focus = color;
            break;
        case QtTextInput::StateError:
            d->border_color.error = color;
            break;
    }
}

void QtTextInput::setBackgroundColor(QtTextInput::InputState state, const QColor &color) {
    Q_D(QtTextInput);
    switch (state) {
        case QtTextInput::StateNormal:
            d->bg_color.normal = color;
            break;
        case QtTextInput::StateDisabled:
            d->bg_color.disabled = color;
            break;
        case QtTextInput::StateFocus:
            d->bg_color.focus = color;
            break;
        case QtTextInput::StateError:
            d->bg_color.error = color;
            break;
    }
}

int QtTextInput::borderWidth() const {
    Q_D(const QtTextInput);
    return d->border_width;
}

Qt::PenStyle QtTextInput::borderStyle() const {
    Q_D(const QtTextInput);
    return d->border_style;
}

int QtTextInput::borderRadius() const {
    Q_D(const QtTextInput);
    return d->border_radius;
}

QColor QtTextInput::borderColor(QtTextInput::InputState state) const {
    Q_D(const QtTextInput);
    switch (state) {
        case StateNormal:
            return d->border_color.normal;
        case StateDisabled:
            return d->border_color.disabled;
        case StateError:
            return d->border_color.error;
        case StateFocus:
            return d->border_color.focus;
        default:
            return {};
    }
}

QColor QtTextInput::backgroundColor(QtTextInput::InputState state) const {
    Q_D(const QtTextInput);
    switch (state) {
        case StateNormal:
            return d->bg_color.normal;
        case StateDisabled:
            return d->bg_color.disabled;
        case StateError:
            return d->bg_color.error;
        case StateFocus:
            return d->bg_color.focus;
        default:
            return {};
    }
}

void QtTextInput::setText(const QString &text) {
    Q_D(QtTextInput);
    d->line_edit->setText(text);
}

void QtTextInput::setPlaceholderText(const QString &text) {
    Q_D(QtTextInput);
    d->line_edit->setPlaceholderText(text);
}

void QtTextInput::setMaxLength(int length) {
    Q_D(QtTextInput);
    d->line_edit->setMaxLength(length);
}

void QtTextInput::setAlignment(Qt::Alignment alignment) {
    Q_D(QtTextInput);
    d->line_edit->setAlignment(alignment);
}

void QtTextInput::setExtraMessage(const QString &message) {
    Q_D(QtTextInput);
    d->info_label->setText(message);
    if (d->has_error) {
        if (d->error_message.isEmpty()) {
            if (d->info_message.isEmpty()) {
                if (message.isEmpty()) {
                    // do nothing
                } else {
                    d->playShowMessageAnimation();
                }
            }
        }
    } else {
        if (d->info_message.isEmpty()) {
            if (message.isEmpty()) {
                // do nothing
            } else {
                d->playShowMessageAnimation();
            }
        }
    }
    d->info_message = message;
}

void QtTextInput::setErrorMessage(const QString &message) {
    Q_D(QtTextInput);
    d->error_label->setText(message);
    if (!d->has_error) {
        d->has_error = true;
        d->playBorderAnimation();
        if (d->info_message.isEmpty()) {
            d->playShowMessageAnimation();
        } else {
            d->info_label->hide();
            d->info_effect->setOpacity(0);
            d->error_label->show();
            d->error_effect->setOpacity(1);
        }
    } else {
        // already has error message
        if (d->error_message.isEmpty()) {
            if (message.isEmpty()) {
                // do nothing
            } else {
                d->playShowMessageAnimation();
            }
        } else {
            if (message.isEmpty()) {
                // do nothing
            } else {
                d->info_label->hide();
                d->info_effect->setOpacity(0);
                d->error_label->show();
                d->error_effect->setOpacity(1);
            }
        }
    }
    d->error_message = message;
}

void QtTextInput::setValidator(const QValidator *validator) {
    Q_D(QtTextInput);
    d->line_edit->setValidator(validator);
}

void QtTextInput::setLeftButton(QAbstractButton *button) {
    Q_D(QtTextInput);
    if (d->left_button) {
        d->input_layout->removeWidget(d->left_button);
        d->left_button->disconnect(this);
        d->left_button->setParent(nullptr);
        d->left_button->deleteLater();
    }
    d->left_button = button;
    d->input_layout->insertWidget(0, button);
    connect(d->left_button, &QAbstractButton::clicked, this, &QtTextInput::leftButtonClicked);
}

void QtTextInput::setRightButton(QAbstractButton *button) {
    Q_D(QtTextInput);
    if (d->right_button) {
        d->input_layout->removeWidget(d->right_button);
        d->right_button->disconnect(this);
        d->right_button->setParent(nullptr);
        d->right_button->deleteLater();
    }
    d->right_button = button;
    d->input_layout->addWidget(button);
    connect(d->right_button, &QAbstractButton::clicked, this, &QtTextInput::rightButtonClicked);
}

void QtTextInput::setEchoMode(QLineEdit::EchoMode mode) {
    Q_D(QtTextInput);
    d->line_edit->setEchoMode(mode);
}

void QtTextInput::setReadOnly(bool readOnly) {
    Q_D(QtTextInput);
    if (this->isReadOnly() == readOnly) return;
    d->line_edit->setReadOnly(readOnly);
    if (d->copy_on_read_only) {
        if (readOnly) {
            d->old_cursor = this->cursor();
            this->setCursor(Qt::PointingHandCursor);
            d->line_edit->setCursor(Qt::PointingHandCursor);
        } else {
            this->setCursor(d->old_cursor);
            d->line_edit->setCursor(Qt::IBeamCursor);
        }
    }
}

void QtTextInput::setCopyOnReadOnly(bool enable) {
    Q_D(QtTextInput);
    if (d->copy_on_read_only == enable) return;
    d->copy_on_read_only = enable;
    if (this->isReadOnly()) {
        if (enable) {
            d->old_cursor = this->cursor();
            this->setCursor(Qt::PointingHandCursor);
            d->line_edit->setCursor(Qt::PointingHandCursor);
        } else {
            this->setCursor(d->old_cursor);
            d->line_edit->setCursor(Qt::IBeamCursor);
        }
    }
}

QString QtTextInput::text() const {
    Q_D(const QtTextInput);
    return d->line_edit->text();
}

QString QtTextInput::placeholderText() const {
    Q_D(const QtTextInput);
    return d->line_edit->placeholderText();
}

int QtTextInput::maxLength() const {
    Q_D(const QtTextInput);
    return d->line_edit->maxLength();
}

Qt::Alignment QtTextInput::alignment() const {
    Q_D(const QtTextInput);
    return d->line_edit->alignment();
}

QString QtTextInput::extraMessage() const {
    Q_D(const QtTextInput);
    return d->info_message;
}

QString QtTextInput::errorMessage() const {
    Q_D(const QtTextInput);
    if (d->has_error) return d->error_message;
    return {};
}

bool QtTextInput::hasError() const {
    Q_D(const QtTextInput);
    return d->has_error;
}

const QValidator *QtTextInput::validator() const {
    Q_D(const QtTextInput);
    return d->line_edit->validator();
}

QAbstractButton *QtTextInput::leftButton() const {
    Q_D(const QtTextInput);
    return d->left_button;
}

QAbstractButton *QtTextInput::rightButton() const {
    Q_D(const QtTextInput);
    return d->right_button;
}

QLineEdit::EchoMode QtTextInput::echoMode() const {
    Q_D(const QtTextInput);
    return d->line_edit->echoMode();
}

bool QtTextInput::isReadOnly() const {
    Q_D(const QtTextInput);
    return d->line_edit->isReadOnly();
}

bool QtTextInput::isCopyOnReadOnly() const {
    Q_D(const QtTextInput);
    return d->copy_on_read_only;
}

void QtTextInput::selectAll() {
    Q_D(const QtTextInput);
    d->line_edit->selectAll();
}

void QtTextInput::clearExtraMessage() {
    Q_D(QtTextInput);
    if (d->has_error) {
        // error has higher priority
        if (d->error_message.isEmpty()) {
            if (d->info_message.isEmpty()) {
                // do nothing
            } else {
                // in this case, extra message is shown, so we need to hide it
                d->playHideMessageAnimation();
            }
        }
    } else {
        if (d->info_message.isEmpty()) {
            // do nothing
        } else {
            // no error, extra message is shown
            d->playHideMessageAnimation();
        }
    }
    // clear message only, do not clear QLabel which shows the message. It will be used for animation
    d->info_message.clear();
}

void QtTextInput::clearErrorMessage() {
    Q_D(QtTextInput);
    if (d->has_error) {
        d->has_error = false;
        d->playBorderAnimation();
        if (d->error_message.isEmpty()) {
            // do nothing
        } else {
            if (d->info_message.isEmpty()) {
                // hide error message
                d->playHideMessageAnimation();
            } else {
                d->error_label->hide();
                d->error_effect->setOpacity(0);
                d->info_label->show();
                d->info_effect->setOpacity(1);
            }
        }
    }
    // clear message only, do not clear QLabel which shows the message. It will be used for animation
    d->has_error = false;
    d->error_message.clear();
}

QSize QtTextInput::sizeHint() const {
    Q_D(const QtTextInput);
    return d->main_layout->sizeHint();
}

QSize QtTextInput::minimumSizeHint() const {
    Q_D(const QtTextInput);
    auto min_width = 150;
    bool has_text = false;
    if ((d->has_error && !d->error_message.isEmpty()) || !d->info_message.isEmpty()) {
        has_text = true;
    }
    return {min_width, has_text ? kDefaultHeight : kHeightWithMessage};
}

void QtTextInput::showEvent(QShowEvent *event) {
    Q_D(QtTextInput);
    QWidget::showEvent(event);
}

void QtTextInput::changeEvent(QEvent *event) {
    Q_D(QtTextInput);
    switch (event->type()) {
        case QEvent::EnabledChange:
            d->playBorderAnimation();
            d->playBackgroundAnimation();
        default:
            break;
    }
    QWidget::changeEvent(event);
}

void QtTextInput::focusInEvent(QFocusEvent *event) {
    Q_D(QtTextInput);
    d->line_edit->setFocus();
    QWidget::focusInEvent(event);
}

void QtTextInput::paintEvent(QPaintEvent *event) {
    Q_D(QtTextInput);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto ev_width = event->rect().width();
    if (ev_width != this->width()) {
        // beam blink, update background only
        // or line_edit hover/focus
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(d->p_bg);
        painter.drawRect(event->rect());
        painter.restore();
        return;
    }

    painter.save();
    auto pen = painter.pen();
    pen.setWidth(this->borderWidth());
    pen.setStyle(this->borderStyle());
    pen.setColor(d->p_border);
    painter.setPen(pen);

    painter.setBrush(d->p_bg);

    // draw rounded rect
    auto rect = this->rect();
    rect.setHeight(kDefaultHeight);
//    qDebug() << "event->rect()=" << event->rect() << "rect=" << rect;
    painter.drawRoundedRect(rect, this->borderRadius(), this->borderRadius());

    painter.restore();
}

void QtTextInput::mousePressEvent(QMouseEvent *event) {
    Q_D(QtTextInput);
    QWidget::mousePressEvent(event);
}

void QtTextInput::mouseReleaseEvent(QMouseEvent *event) {
    Q_D(QtTextInput);
    if (this->underMouse()) {
        d->line_edit->setFocus();
    }
    QWidget::mouseReleaseEvent(event);
}

bool QtTextInput::eventFilter(QObject *watched, QEvent *event) {
    Q_D(QtTextInput);
    if (watched == d->line_edit) {
        switch (event->type()) {
            case QEvent::FocusIn:
                d->playBorderAnimation();
                d->playBackgroundAnimation();
                if (this->isReadOnly() && d->copy_on_read_only) {
                    d->copyAndSelectAll();
                }
                break;
            case QEvent::FocusOut:
                d->playBorderAnimation();
                d->playBackgroundAnimation();
                return false;
            default:
                break;
        }
    }
    return false;
}

QtTextInputPrivate::QtTextInputPrivate(QtTextInput *q) : q_ptr(q) {
    this->line_edit = new QLineEdit;
    this->line_edit->setStyleSheet(kLineEditStyle);

    this->info_label = new QLabel;
    this->info_label->setStyleSheet(kInfoMessageStyle);
    this->info_label->hide();

    this->info_effect = new QGraphicsOpacityEffect(this->info_label);
    this->info_effect->setOpacity(0);
    this->info_label->setGraphicsEffect(this->info_effect);

    this->error_label = new QLabel;
    this->error_label->setStyleSheet(kErrorMessageStyle);
    this->error_label->hide();

    this->error_effect = new QGraphicsOpacityEffect(this->error_label);
    this->error_effect->setOpacity(0);
    this->error_label->setGraphicsEffect(this->error_effect);

    this->main_layout = new QVBoxLayout;
    this->main_layout->setContentsMargins(0, 0, 0, 0);
    this->main_layout->setSpacing(kMessageSpacing);

    this->input_container = new QWidget;
    this->input_container->setFixedHeight(kDefaultHeight);

    this->input_layout = new QHBoxLayout(this->input_container);
    this->input_layout->setContentsMargins(WITH_BORDER_INPUT_MARGINS);
    this->input_layout->setSpacing(kInputSpacing);

    this->input_layout->addWidget(this->line_edit);

    this->main_layout->addWidget(this->input_container);
    this->main_layout->addWidget(this->info_label);
    this->main_layout->addWidget(this->error_label);

    this->main_layout->setStretch(0, kDefaultHeight);
    this->main_layout->setStretch(1, kHeightWithMessage - kDefaultHeight);
    this->main_layout->setStretch(2, kHeightWithMessage - kDefaultHeight);

    this->line_edit->installEventFilter(q);

    border_color.normal = kNormalBorderColor;
    border_color.error = kErrorBorderColor;
    border_color.focus = kFocusBorderColor;
    border_color.disabled = kDisabledBorderColor;
    p_border = border_color.normal;

    bg_color.normal = kNormalBgColor;
    bg_color.error = kErrorBgColor;
    bg_color.focus = kFocusBgColor;
    bg_color.disabled = kDisabledBgColor;
    p_bg = bg_color.normal;
}

QtTextInputPrivate::~QtTextInputPrivate() {
    this->line_edit->deleteLater();
    this->info_label->deleteLater();
    this->error_label->deleteLater();
}

void QtTextInputPrivate::playShowMessageAnimation() {
    Q_Q(QtTextInput);
    if (!this->info_label->text().isEmpty()) this->info_label->show();
    if (!this->error_label->text().isEmpty()) this->error_label->show();
    this->createOrStopHeightAnim(true);
    this->msg_animation->start();
}

void QtTextInputPrivate::playHideMessageAnimation() {
    Q_Q(QtTextInput);
    this->createOrStopHeightAnim(false);
    this->msg_animation->start();
}

void QtTextInputPrivate::playBorderAnimation() {
    Q_Q(QtTextInput);
    this->createOrStopColorAnim(this->bd_animation, this->p_border);
    if (!q->isEnabled()) {
        this->bd_animation->setEndValue(this->border_color.disabled);
    } else if (this->has_error) {
        this->bd_animation->setEndValue(this->border_color.error);
    } else if (q->hasFocus() || this->line_edit->hasFocus()) {
        this->bd_animation->setEndValue(this->border_color.focus);
    } else {
        this->bd_animation->setEndValue(this->border_color.normal);
    }
    this->bd_animation->start();
}

void QtTextInputPrivate::playBackgroundAnimation() {
    Q_Q(QtTextInput);
    this->createOrStopColorAnim(this->bg_animation, this->p_bg);
    if (!q->isEnabled()) {
        this->bg_animation->setEndValue(this->bg_color.disabled);
    } else if (this->has_error) {
        this->bg_animation->setEndValue(this->bg_color.error);
    } else if (q->hasFocus() || this->line_edit->hasFocus()) {
        this->bg_animation->setEndValue(this->bg_color.focus);
    } else {
        this->bg_animation->setEndValue(this->bg_color.normal);
    }
    this->bg_animation->start();
}

void QtTextInputPrivate::copyAndSelectAll() {
    Q_Q(QtTextInput);
    auto text = this->line_edit->text();
    if (text.isEmpty()) return;

    // using Qt::QueuedConnection to avoid not working
    QMetaObject::invokeMethod(q, &QtTextInput::selectAll, Qt::QueuedConnection);
    auto *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    emit q->textCopied(text);
}

void QtTextInputPrivate::createOrStopColorAnim(QVariantAnimation *&anim, QColor &target) {
    Q_Q(QtTextInput);
    if (anim) {
        anim->stop();
        anim->setStartValue(target);
    } else {
        anim = new QVariantAnimation(q);
        anim->setDuration(kAnimationDuration);
        anim->setStartValue(target);
        QObject::connect(anim, &QVariantAnimation::valueChanged, q, [q, &target](const QVariant &value) {
            target = value.value<QColor>();
            q->update();
        });
    }
}

void QtTextInputPrivate::createOrStopHeightAnim(bool isShow) {
    Q_Q(QtTextInput);
    if (this->msg_animation) {
        this->msg_animation->stop();
        this->msg_animation->deleteLater();
    }
    this->msg_animation = new QParallelAnimationGroup(q);

    auto *opacity_anim = new QPropertyAnimation(this->msg_animation);
    opacity_anim->setPropertyName("opacity");
    if (!this->error_label->text().isEmpty()) opacity_anim->setTargetObject(this->error_effect);
    if (!this->info_label->text().isEmpty()) opacity_anim->setTargetObject(this->info_effect);
    opacity_anim->setDuration(kAnimationDuration);
    opacity_anim->setStartValue(isShow ? 0.0 : 1.0);
    opacity_anim->setEndValue(isShow ? 1.0 : 0.0);

    auto *size_animation = new QVariantAnimation(this->msg_animation);
    size_animation->setDuration(kAnimationDuration);
    size_animation->setStartValue(q->height());
    size_animation->setEndValue(isShow ? kHeightWithMessage : kDefaultHeight);
    QObject::connect(size_animation, &QVariantAnimation::valueChanged, q, [this, q](const QVariant &value) {
        auto const h = value.toInt();
        this->error_label->resize(error_label->width(), h - kDefaultHeight);
        this->info_label->resize(error_label->width(), h - kDefaultHeight);
        q->resize(q->width(), h);
    });

    this->msg_animation->addAnimation(opacity_anim);
    this->msg_animation->addAnimation(size_animation);

    if (!isShow) {
        QObject::connect(this->msg_animation, &QAbstractAnimation::finished, q, [this] {
            this->clearAndHideMessages();
        });
    }
}

void QtTextInputPrivate::clearAndHideMessages() {
    Q_Q(QtTextInput);
    this->info_label->clear();
    this->error_label->clear();
    this->info_label->hide();
    this->error_label->hide();
}

FNRICE_QT_WIDGETS_END_NAMESPACE
