import {Component, Inject, OnDestroy, OnInit} from '@angular/core';
import {ManagementService} from './services/management/management.service';
import {MatSlideToggleChange} from '@angular/material/slide-toggle';
import {DOCUMENT} from '@angular/common';
import {ActionModel, ActionType, StateModel} from './models/effect.model';
import {WebsocketService} from './services/websocket/websocket.service';
import {debounceTime, Subject, takeUntil} from 'rxjs';
import {MatDialog, MatDialogRef} from '@angular/material/dialog';
import {ColorPickerComponent} from './components/color-picker/color-picker.component';
import {MatCheckboxChange} from '@angular/material/checkbox';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit, OnDestroy {
  title = 'Гирлянда';
  private static readonly DARK_THEME_CLASS = 'dark-theme';

  get isDarkTheme() {
    return this._darkTheme;
  }

  set isDarkTheme(value: boolean) {
    this._darkTheme = value;
    if (value) {
      this.document.documentElement.classList.add(AppComponent.DARK_THEME_CLASS);
    } else {
      this.document.documentElement.classList.remove(AppComponent.DARK_THEME_CLASS);
    }
    try {
      window.localStorage[AppComponent.DARK_THEME_CLASS] = value;
    }
    catch {
    }
  }

  public state = new StateModel();

  public power: ActionModel;
  public bright_5: ActionModel;
  public bright_25: ActionModel;
  public bright_50: ActionModel;
  public bright_75: ActionModel;
  public bright_100: ActionModel;

  public color_black: ActionModel;
  public color_white: ActionModel;
  public color_r: ActionModel;
  public color_g: ActionModel;
  public color_b: ActionModel;
  public color_y: ActionModel;
  public color_c: ActionModel;
  public color_m: ActionModel;
  public color_set: ActionModel;
  public color_user: ActionModel;

  private _darkTheme: boolean = false;

  private destroy$ = new Subject();

  private colorDialogRef: MatDialogRef<ColorPickerComponent> | null = null;

  ngOnInit() {
  }

  changeTheme($event: MatSlideToggleChange) {
    this.isDarkTheme = $event.checked;
  }

  setDarkTheme(isDark: boolean) {
    this.isDarkTheme = isDark;
  }

  isDisabled(): boolean {
    return !this.state.power || !this.socketService.isConnected;
  }

  ngOnDestroy() {
    this.destroy$.next(true);
    this.destroy$.complete();
  }

  constructor(@Inject(DOCUMENT) private document: Document,
              public socketService: WebsocketService,
              public effectService: ManagementService,
              private dialog: MatDialog) {
    this.isDarkTheme = window.localStorage[AppComponent.DARK_THEME_CLASS] === 'true' || false;

    function checkRange(val: any, min: number, max: number) {
      let res = false;
      if (typeof val === 'number') {
        const v = Number(val);
        return v >= min && v <= max;
      }
      return res;
    }

    this.power = new ActionModel({
      mode: ActionType.POWER,
      onoff: ((value: any) => value ? 'ON' : 'OFF'),
      set: ((value: any) => value ? 'PWR:OFF' : 'PWR:ON')
    });
    this.bright_5 = new ActionModel({
      mode: ActionType.BRIGHT,
      submode: 'bright_5',
      onoff: ((value: any) => {
        return checkRange(value, 0, 13) ? 'ON' : 'OFF';
      }),
      set: 'BR:12'
    });
    this.bright_25 = new ActionModel({
      mode: ActionType.BRIGHT,
      submode: 'bright_25',
      onoff: ((value: any) => {
        return checkRange(value, 14, 64) ? 'ON' : 'OFF';
      }),
      set: 'BR:64'
    });
    this.bright_50 = new ActionModel({
      mode: ActionType.BRIGHT,
      submode: 'bright_50',
      onoff: ((value: any) => {
        return checkRange(value, 65, 128) ? 'ON' : 'OFF';
      }),
      set: 'BR:128'
    });
    this.bright_75 = new ActionModel({
      mode: ActionType.BRIGHT,
      submode: 'bright_75',
      onoff: ((value: any) => {
        return checkRange(value, 129, 192) ? 'ON' : 'OFF';
      }),
      set: 'BR:192'
    });
    this.bright_100 = new ActionModel({
      mode: ActionType.BRIGHT,
      submode: 'bright_100',
      onoff: ((value: any) => {
        return checkRange(value, 193, 255) ? 'ON' : 'OFF';
      }),
      set: 'BR:255'
    });

    this.color_black = new ActionModel({
      id: 99,
      mode: ActionType.COLOR,
      submode: 'color_black',
      set: 'DO:99'
    });
    this.color_white = new ActionModel({
      id: 100,
      mode: ActionType.COLOR,
      submode: 'color_white',
      set: 'DO:100'
    });
    this.color_r = new ActionModel({
      id: 101,
      mode: ActionType.COLOR,
      submode: 'color_r',
      set: 'DO:101'
    });
    this.color_g = new ActionModel({
      id: 102,
      mode: ActionType.COLOR,
      submode: 'color_g',
      set: 'DO:102'
    });
    this.color_b = new ActionModel({
      id: 103,
      mode: ActionType.COLOR,
      submode: 'color_b',
      set: 'DO:103'
    });
    this.color_y = new ActionModel({
      id: 104,
      mode: ActionType.COLOR,
      submode: 'color_y',
      set: 'DO:104'
    });
    this.color_c = new ActionModel({
      id: 105,
      mode: ActionType.COLOR,
      submode: 'color_c',
      set: 'DO:105'
    });
    this.color_m = new ActionModel({
      id: 106,
      mode: ActionType.COLOR,
      submode: 'color_m',
      set: 'DO:106'
    });
    this.color_set = new ActionModel({
      mode: ActionType.COLOR,
      submode: 'color_set',
      set: (value: any) => {
        this.ShowColorSelector();
      }
    });
    this.color_user = new ActionModel({
      id: 107,
      mode: ActionType.COLOR,
      submode: 'color_user',
      set: 'DO:107'
    });

    // Режимы включения выбранного цвета ленты
    this.effectService.actions.push(this.color_black);
    this.effectService.actions.push(this.color_white);
    this.effectService.actions.push(this.color_r);
    this.effectService.actions.push(this.color_g);
    this.effectService.actions.push(this.color_b);
    this.effectService.actions.push(this.color_y);
    this.effectService.actions.push(this.color_c);
    this.effectService.actions.push(this.color_m);
    this.effectService.actions.push(this.color_user);

    this.effectService.state$
      .pipe(takeUntil(this.destroy$), debounceTime(100))
      .subscribe((state: StateModel) => {
        this.state = state;
      });
  }

  private ShowColorSelector() {
    this.colorDialogRef = this.dialog.open(ColorPickerComponent, {
      panelClass: 'color-dialog-panel',
      data: { color: this.state.color }
    });

    this.colorDialogRef.afterClosed().subscribe((result) => {
      if (result) {
        const command = `RGB:${result.r}:${result.g}:${result.b}`;
        this.socketService.sendText(command);
      }
    });
  }

  toggleRandomMode($event: MatCheckboxChange) {
    const command = `RND:${$event.checked ? 'ON' : 'OFF'}`;
    this.socketService.sendText(command);
  }

}
