// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_CHROME_BUBBLE_MANAGER_H_
#define CHROME_BROWSER_UI_CHROME_BUBBLE_MANAGER_H_

#include "base/macros.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/bubble/bubble_manager.h"
#include "content/public/browser/web_contents_observer.h"

class TabStripModel;

class ChromeBubbleManager : public BubbleManager,
                            public TabStripModelObserver,
                            public content::WebContentsObserver {
 public:
  explicit ChromeBubbleManager(TabStripModel* tab_strip_model);
  ~ChromeBubbleManager() override;

  // TabStripModelObserver:
  void TabDetachedAt(content::WebContents* contents, int index) override;
  void TabDeactivated(content::WebContents* contents) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;

  // content::WebContentsObserver:
  void DidToggleFullscreenModeForTab(bool entered_fullscreen) override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;

 private:
  class ChromeBubbleMetrics : public BubbleManager::BubbleManagerObserver {
   public:
    ChromeBubbleMetrics() {}
    ~ChromeBubbleMetrics() override {}

    // BubbleManager::BubbleManagerObserver:
    void OnBubbleNeverShown(BubbleReference bubble) override;
    void OnBubbleClosed(BubbleReference bubble,
                        BubbleCloseReason reason) override;

   private:
    DISALLOW_COPY_AND_ASSIGN(ChromeBubbleMetrics);
  };

  TabStripModel* tab_strip_model_;
  ChromeBubbleMetrics chrome_bubble_metrics_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBubbleManager);
};

#endif  // CHROME_BROWSER_UI_CHROME_BUBBLE_MANAGER_H_
