import {NgModule} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';
import {AppComponent} from './app.component';
import {BrowserAnimationsModule} from '@angular/platform-browser/animations';
import {WebsocketModule} from './services/websocket/websocket.module';
import {ManagementService} from "./services/management/management.service";
import {MatIconModule} from "@angular/material/icon";
import {MatButtonModule} from "@angular/material/button";
import {MatMenuModule} from "@angular/material/menu";
import {MatToolbarModule} from "@angular/material/toolbar";
import {MatSlideToggleModule} from "@angular/material/slide-toggle";
import {MAT_TOOLTIP_DEFAULT_OPTIONS, MatTooltipDefaultOptions, MatTooltipModule} from "@angular/material/tooltip";
import {ColorPickerComponent} from './components/color-picker/color-picker.component';
import {ActionComponent} from './components/action/action.component';
import {BrightnessSliderComponent} from './components/brightness-slider/brightness-slider.component';
import {MatSliderModule} from "@angular/material/slider";
import {MAT_DIALOG_DATA, MatDialogModule} from "@angular/material/dialog";
import {DragDropModule} from "@angular/cdk/drag-drop";
import {ColorGithubModule} from "ngx-color/github";
import {ColorChromeModule} from "ngx-color/chrome";
import {ColorHueModule} from "ngx-color/hue";
import {SaturationModule} from "ngx-color";
import {FormsModule} from "@angular/forms";
import {MatCheckboxModule} from "@angular/material/checkbox";
import {EffectComponent} from './components/effect/effect.component';
import {EffectParamsComponent} from './components/effect-params/effect-params.component';

export const customTooltipDefaults: MatTooltipDefaultOptions = {
  showDelay: 1000,
  hideDelay: 100,
  touchendHideDelay: 100
}

@NgModule({
  declarations: [
    AppComponent,
    ColorPickerComponent,
    ActionComponent,
    BrightnessSliderComponent,
    EffectComponent,
    EffectParamsComponent,
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    WebsocketModule,
    MatIconModule,
    MatButtonModule,
    MatMenuModule,
    MatToolbarModule,
    ColorGithubModule,
    MatSlideToggleModule,
    MatTooltipModule,
    MatSliderModule,
    MatDialogModule,
    DragDropModule,
    ColorChromeModule,
    SaturationModule,
    FormsModule,
    MatCheckboxModule,
    ColorHueModule,
  ],
  providers: [
    {provide: ManagementService.management, useClass: ManagementService},
    {provide: MAT_TOOLTIP_DEFAULT_OPTIONS, useValue: customTooltipDefaults},
    {provide: MAT_DIALOG_DATA, useValue: []},
  ],
  bootstrap: [AppComponent]
})
export class AppModule {
}
