import React, { Component, useEffect } from 'react';
import './AlertComponent.scss';

class AlertComponent extends Component<any, any> {

  public timeout: number = 3;
  public type: string;
  public content: string;
  public dismiss: Function;

  constructor(props) {
    super(props);
    this.state = {
      hide: false
    };
    this.timeout = props.timeout;
    this.content = props.content;
    this.type = props.type;
    this.dismiss = props.dismiss;
  }

  componentDidMount = () => {
    const timer = setTimeout(() => {
      if (this.dismiss != null) {
        this.dismiss()
      }
      this.setState({
        hide: true
      })
    }, this.timeout * 1000)

  }
  getClassNames = () =>{
    return ['alert','alert-'+this.type].join(" ");
  }
  
  render = () => {
    return (
      <div hidden={this.state.hide} className={this.getClassNames()} role="alert">
        {this.content}
      </div>
    );
  }

}
export default AlertComponent;
