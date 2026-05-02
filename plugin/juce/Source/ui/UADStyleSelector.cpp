#include "UADStyleSelector.h"

namespace bc2000dl::ui
{
    UADStyleSelector::UADStyleSelector (juce::AudioProcessorValueTreeState& a,
                                        const juce::String& pid,
                                        const juce::StringArray& labels,
                                        const juce::String& title)
        : apvts (a), paramID (pid), titleText (title)
    {
        // Hidden ComboBox används som "source of truth" via APVTS attachment
        for (int i = 0; i < labels.size(); ++i)
            hiddenCombo.addItem (labels[i], i + 1);

        const int radioId = juce::Random::getSystemRandom().nextInt (10000) + 100;
        for (int i = 0; i < labels.size(); ++i)
        {
            auto* b = new juce::TextButton (labels[i]);
            b->setRadioGroupId (radioId);
            b->setClickingTogglesState (true);
            b->setConnectedEdges (
                  (i > 0 ? juce::Button::ConnectedOnLeft : 0)
                | (i < labels.size() - 1 ? juce::Button::ConnectedOnRight : 0));
            b->onClick = [this, i] { setSelectedIndex (i); };
            addAndMakeVisible (b);
            buttons.add (b);
        }

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, paramID, hiddenCombo);

        // När hidden combo ändras → uppdatera knapp-radio-state
        hiddenCombo.onChange = [this] { updateButtonsFromCombo(); };
        updateButtonsFromCombo();
    }

    UADStyleSelector::~UADStyleSelector()
    {
        hiddenCombo.onChange = nullptr;
    }

    void UADStyleSelector::updateButtonsFromCombo()
    {
        const int idx = hiddenCombo.getSelectedItemIndex();
        if (idx >= 0 && idx < buttons.size())
        {
            if (! buttons[idx]->getToggleState())
                buttons[idx]->setToggleState (true, juce::dontSendNotification);
        }
    }

    void UADStyleSelector::setSelectedIndex (int idx)
    {
        hiddenCombo.setSelectedItemIndex (idx, juce::sendNotificationSync);
    }

    void UADStyleSelector::paint (juce::Graphics& g)
    {
        if (titleText.isNotEmpty())
        {
            g.setColour (juce::Colour (0xffd0cdc4));
            g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText (titleText,
                        getLocalBounds().removeFromTop (12),
                        juce::Justification::centred, false);
        }
    }

    void UADStyleSelector::resized()
    {
        auto bounds = getLocalBounds();
        if (titleText.isNotEmpty()) bounds.removeFromTop (12);
        const int n = buttons.size();
        if (n == 0) return;
        const int w = bounds.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            const int x = bounds.getX() + i * w;
            const int width = (i == n - 1) ? bounds.getRight() - x : w;
            buttons[i]->setBounds (x, bounds.getY(), width, bounds.getHeight());
        }
    }
}
