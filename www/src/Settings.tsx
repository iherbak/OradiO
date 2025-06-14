import React, { Component } from 'react';
import './Settings.scss';
import { SettingsField, SettingsProps } from 'models';
import { ApiService } from 'api.service';
import AlertComponent from 'AlertComponent';

class Settings extends Component<any, any> {

  private apiService: ApiService;

  constructor(public props: SettingsProps) {
    super(props);
    this.apiService = new ApiService();
  }

  updatesettings = (event, settingsField: SettingsField) => {
    var params = this.props.settings;
    switch (settingsField) {
      case SettingsField.stassid: {
        params.wifi.stassid = event.target.value;
        break;
      }
      case SettingsField.stapassword: {
        params.wifi.stapassword = event.target.value;
        break;
      }
      case SettingsField.startasap:{
        params.wifi.startasap = !params.wifi.startasap;
        break;
      }
      case SettingsField.rows: {
        params.display.rows = event.target.value;
        break;
      }
      case SettingsField.cols: {
        params.display.cols = event.target.value;
        break;
      }
      case SettingsField.sdapin: {
        params.display.sdapin = event.target.value;
        break;

      }
      case SettingsField.sclpin: {
        params.display.sclpin = event.target.value;
        break;
      }
      case SettingsField.dreqpin: {
        params.vs1053.dreqpin = event.target.value;
        break;
      }
      case SettingsField.xcspin: {
        params.vs1053.xcspin = event.target.value;
        break;
      }
      case SettingsField.xdcspin: {
        params.vs1053.xdcspin = event.target.value;
        break;
      }
      case SettingsField.resetpin: {
        params.vs1053.resetpin = event.target.value;
        break;
      }
      case SettingsField.mosipin: {
        params.vs1053.mosipin = event.target.value;
        break;
      }
      case SettingsField.misopin: {
        params.vs1053.misopin = event.target.value;
        break;
      }
      case SettingsField.sclkpin: {
        params.vs1053.sclkpin = event.target.value;
        break;
      }
      case SettingsField.startvolume:{
        params.vs1053.startvolume = event.target.value;
      }
    }
    this.props.setSettings(params)
  }

  save = () => {
    this.apiService.saveSettings(this.props.settings).then(() => {
      this.props.getSettings();
      this.props.addAlert({ type: 'info', content: "Mentés kész", timeout: 3, dismiss: this.props.clearAlerts });
    });
  }

  restart = () => {
    this.apiService.restart().then(() => {
    });
  }

  render = () => {
    return (
      <>
        <h3>Beállítások</h3>
        <div className="cardContainer">
          <div className="card menu">
            <div className="card-header">
              Műveletek
            </div>
            <div className="card-body">
              <div className='normal-state'>
                <button title='Mentés' className='btn btn-outline-primary'><i className="bi-floppy" onClick={() => this.save()}></i></button>
                <button title='Újraindítás' className='btn btn-outline-light'><i className="bi-arrow-clockwise" onClick={() => this.restart()}></i></button>
              </div>
              {this.props.state.alerts.map(alert => {
                return <AlertComponent key={alert.type} timeout={alert.timeout} content={alert.content} type={alert.type} dismiss={alert.dismiss}></AlertComponent>
              })}
            </div>
          </div>

          <div className="card setting">
            <div className="card-header">
              Wifi
            </div>
            <div className="card-body">
              <span>STA SSID</span><input type="text" value={this.props.settings?.wifi?.stassid} onChange={(event) => this.updatesettings(event, SettingsField.stassid)}></input>
              <span>STA jelszó</span><input type="text" value={this.props.settings?.wifi?.stapassword} onChange={(event) => this.updatesettings(event, SettingsField.stapassword)}></input>
              <span>AP SSID</span><input type="text" value={this.props.settings?.wifi?.apssid} disabled></input>
              <span>AP jelszó</span><input type="text" value={this.props.settings?.wifi?.appassword} disabled></input>
              <span>Indítás AP módban</span><input type="checkbox" checked={this.props.settings?.wifi?.startasap} onChange={(event) => this.updatesettings(event, SettingsField.startasap)}></input>
            </div>
          </div>
          <div className="card setting">
            <div className="card-header">
              Display
            </div>
            <div className="card-body">
              <span>Sorok</span><input type="number" value={this.props.settings?.display?.rows} onChange={(event) => this.updatesettings(event, SettingsField.rows)}></input>
              <span>Oszlopok</span><input type="number" value={this.props.settings?.display?.cols} onChange={(event) => this.updatesettings(event, SettingsField.cols)}></input>
              <span>Sda pin</span><input type="number" value={this.props.settings?.display?.sdapin} onChange={(event) => this.updatesettings(event, SettingsField.sdapin)}></input>
              <span>Scl pin</span><input type="number" value={this.props.settings?.display?.sclpin} onChange={(event) => this.updatesettings(event, SettingsField.sclpin)}></input>
            </div>
          </div>
          <div className="card setting">
            <div className="card-header">
              Vs1053
            </div>
            <div className="card-body">
              <span>Dreq pin</span><input type="number" value={this.props.settings?.vs1053?.dreqpin} onChange={(event) => this.updatesettings(event, SettingsField.dreqpin)}></input>
              <span>XCS pin</span><input type="number" value={this.props.settings?.vs1053?.xcspin} onChange={(event) => this.updatesettings(event, SettingsField.xcspin)}></input>
              <span>XDCS pin</span><input type="number" value={this.props.settings?.vs1053?.xdcspin} onChange={(event) => this.updatesettings(event, SettingsField.xdcspin)}></input>
              <span>Reset pin</span><input type="number" value={this.props.settings?.vs1053?.resetpin} onChange={(event) => this.updatesettings(event, SettingsField.resetpin)}></input>
              <span>MOSI pin</span><input type="number" value={this.props.settings?.vs1053?.mosipin} onChange={(event) => this.updatesettings(event, SettingsField.mosipin)}></input>
              <span>MISO pin</span><input type="number" value={this.props.settings?.vs1053?.misopin} onChange={(event) => this.updatesettings(event, SettingsField.misopin)}></input>
              <span>SCLK pin</span><input type="number" value={this.props.settings?.vs1053?.sclkpin} onChange={(event) => this.updatesettings(event, SettingsField.sclkpin)}></input>
              <span>Startup volume</span><input type="number" value={this.props.settings?.vs1053?.startvolume} onChange={(event) => this.updatesettings(event, SettingsField.startvolume)}></input>
            </div>
          </div>

        </div>
      </>
    );
  }
}
export default Settings;
