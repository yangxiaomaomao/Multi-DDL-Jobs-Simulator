/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef WAVEFORM_GENERATOR_H
#define WAVEFORM_GENERATOR_H

#include "spectrum-channel.h"
#include "spectrum-phy.h"
#include "spectrum-value.h"

#include <ns3/event-id.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/nstime.h>
#include <ns3/packet.h>
#include <ns3/trace-source-accessor.h>

namespace ns3
{

class AntennaModel;

/**
 * @ingroup spectrum
 *
 * Simple SpectrumPhy implementation that sends customizable waveform.
 * The generated waveforms have a given Spectrum Power Density and
 * duration (set with the SetResolution()) . The generator activates
 * and deactivates periodically with a given period and with a duty
 * cycle of 1/2.
 *
 * This PHY model supports a single antenna model instance which is
 * used for both transmission and reception (though received signals
 * are discarded by this PHY).
 */
class WaveformGenerator : public SpectrumPhy
{
  public:
    WaveformGenerator();
    ~WaveformGenerator() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from SpectrumPhy
    void SetChannel(Ptr<SpectrumChannel> c) override;
    void SetMobility(Ptr<MobilityModel> m) override;
    void SetDevice(Ptr<NetDevice> d) override;
    Ptr<MobilityModel> GetMobility() const override;
    Ptr<NetDevice> GetDevice() const override;
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override;
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Set the Power Spectral Density used for outgoing waveforms
     *
     * @param txs the Power Spectral Density
     */
    void SetTxPowerSpectralDensity(Ptr<SpectrumValue> txs);

    /**
     * Set the period according to which the WaveformGenerator switches
     * on and off
     *
     * @param period
     */
    void SetPeriod(Time period);

    /**
     *
     * @return the value of the period according to which the WaveformGenerator switches
     * on and off
     */
    Time GetPeriod() const;

    /**
     *
     * @param value the value of the duty cycle
     */
    void SetDutyCycle(double value);

    /**
     *
     * @return the value of the duty cycle
     */
    double GetDutyCycle() const;

    /**
     * set the AntennaModel to be used
     *
     * @param a the Antenna Model
     */
    void SetAntenna(Ptr<AntennaModel> a);

    /**
     * Start the waveform generator
     *
     */
    virtual void Start();

    /**
     * Stop the waveform generator
     *
     */
    virtual void Stop();

  private:
    void DoDispose() override;

    Ptr<MobilityModel> m_mobility;  //!< Mobility model
    Ptr<AntennaModel> m_antenna;    //!< Antenna model
    Ptr<NetDevice> m_netDevice;     //!< Owning NetDevice
    Ptr<SpectrumChannel> m_channel; //!< Channel

    /**
     * Generates a waveform
     */
    virtual void GenerateWaveform();

    Ptr<SpectrumValue> m_txPowerSpectralDensity; //!< Tx PSD
    Time m_period;                               //!< Period
    double m_dutyCycle;                          //!< Duty Cycle (should be in [0,1])
    Time m_startTime;                            //!< Start time
    EventId m_nextWave;                          //!< Next waveform generation event

    TracedCallback<Ptr<const Packet>> m_phyTxStartTrace; //!< TracedCallback: Tx start
    TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;   //!< TracedCallback: Tx end
};

} // namespace ns3

#endif /* WAVEFORM_GENERATOR_H */
